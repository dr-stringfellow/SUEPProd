#include "../interface/GenParticlesFiller.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/Common/interface/RefToPtr.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"

#include "SUEPProd/Auxiliary/interface/PackedValuesExposer.h"
#include "SUEPTree/Utils/interface/PNode.h"

typedef edm::Ptr<reco::GenParticle> GenParticlePtr;
typedef edm::Ptr<pat::PackedGenParticle> PackedGenParticlePtr;

namespace {
  // Track the PNodeWithPtrs that are made from packed collections
  std::unordered_set<const void*> miniaodPacked;
}

struct PNodeWithPtr : public PNode {
  reco::CandidatePtr candPtr{};
  reco::CandidatePtr replacedCandPtr{};
  uint16_t packedPt{0xffff};
  uint16_t packedPhi{0xffff};
  uint16_t packedM{0xffff};

  PNodeWithPtr(GenParticlePtr const& _ptr, std::map<reco::CandidatePtr, PNodeWithPtr*>& _nodeMap, PNode* _mother = 0) {
    auto& inCand(*_ptr);
    pdgId = inCand.pdgId();
    status = inCand.status();
    statusBits = inCand.statusFlags().flags_;
    mass = inCand.mass();
    pt = inCand.pt();
    eta = inCand.eta();
    phi = inCand.phi();
    mother = _mother;

    ownDaughters = false;

    candPtr = _ptr;

    _nodeMap[candPtr] = this;

    for (auto& dref : inCand.daughterRefVector()) {
      GenParticlePtr dptr(edm::refToPtr(dref));

      auto nItr(_nodeMap.find(dptr));

      if (nItr != _nodeMap.end()) {
        // this node is already constructed

        PNode* dnode(nItr->second);

        // protect against cyclic graphs - is dnode my ancestor?
        PNode* p(this);
        while (p) {
          if (p == dnode)
            break;
          p = p->mother;
        }
        if (p) // one of my ancestors is dnode; don't add this node as my daughter
          continue;

        PNode* dmother(dnode->mother);

        if (dmother) {
          // and it's someone's daughter

          if (dmother == this) // mine!?
            continue;

          bool takeCustody(false);
          if (!isHadronic() && dmother->isHadronic())
            takeCustody = true;
          else if (isHadronic() && !dmother->isHadronic())
            takeCustody = false;
          else
            takeCustody = reco::deltaR2(eta, phi, dnode->eta, dnode->phi) < reco::deltaR2(dmother->eta, dmother->phi, dnode->eta, dnode->phi);

          if (takeCustody) {
            dnode->mother = this;
            daughters.push_back(dnode);
            std::vector<PNode*>::iterator dItr(std::find(dmother->daughters.begin(), dmother->daughters.end(), dnode));
            if (dItr != dmother->daughters.end()) // can happen that this daughter is still being pushed into mother's daughter list
              dmother->daughters.erase(dItr);
          }
        }
        else {
          dnode->mother = this;
          daughters.push_back(dnode);
        }
      }
      else
        daughters.push_back(new PNodeWithPtr(dptr, _nodeMap, this));
    }
  }

  PNodeWithPtr(PackedGenParticlePtr const& _ptr, std::map<reco::CandidatePtr, PNodeWithPtr*>& _nodeMap) {
    auto& inCand(*_ptr);
    pdgId = inCand.pdgId();
    status = 1;
    mass = inCand.mass();
    pt = inCand.pt();
    eta = inCand.eta();
    phi = inCand.phi();

    PackedGenParticleExposer exposer(inCand);
    packedPt = exposer.packedPt();
    packedPhi = exposer.packedPhi();
    packedM = exposer.packedM();

    candPtr = _ptr;
    // don't really need to add this to nodeMap, but makes it easy to delete the nodes later
    _nodeMap[candPtr] = this;

    auto motherRef(inCand.motherRef());
    if (motherRef.isNonnull()) {
      mother = _nodeMap.at(reco::CandidatePtr(edm::refToPtr(motherRef)));

      // kick out the existing daughter
      unsigned iD(0);
      for (; iD != mother->daughters.size(); ++iD) {
        auto* d(mother->daughters[iD]);

        double dpt(std::abs(d->pt - pt));
        if (pt == 0. && dpt > 0.1)
          continue;

        if (d->pdgId == pdgId && d->status == 1 && reco::deltaR2(d->eta, d->phi, eta, phi) < 0.0001 && dpt / pt < 0.05) {
          // found a matching candidate, kick it out
          mother->daughters[iD] = this;
          replacedCandPtr = static_cast<PNodeWithPtr*>(d)->candPtr;

          if (dynamic_cast<reco::GenParticle const*>(&*replacedCandPtr)) {
            GenParticlePtr genP(replacedCandPtr);
            if (replacedCandPtr.isNonnull())
              statusBits = genP->statusFlags().flags_;
          }

          delete d;
          _nodeMap.erase(replacedCandPtr);
          break;
        }
      }

      if (iD == mother->daughters.size()) {
        // no one was impersonating me
        mother->daughters.push_back(this);
      }
    }
  }

  void fillSUEP(suep::GenParticleCollection& _outParticles, ObjectMap<reco::Candidate, suep::GenParticle>& _map, int parentIdx = -1) const {
    auto& outParticle(_outParticles.create_back());
    int myidx(_outParticles.size() - 1);

    if (packedPt < uint16_t(0xffff)) {
      outParticle.packedPt = packedPt;
      // should probably have an interface at PackedParticle
      outParticle.packedEta = std::round(candPtr->eta() / 6.0f * std::numeric_limits<Short_t>::max());
      outParticle.packedPhi = packedPhi;
      outParticle.packedM = packedM;
    }
    else
      fillP4(outParticle, *candPtr);

    outParticle.pdgid = pdgId;
    outParticle.finalState = (status == 1);
    outParticle.miniaodPacked = (miniaodPacked.find(this) != miniaodPacked.end());
    outParticle.statusFlags = statusBits.to_ulong();
    outParticle.parent.idx() = parentIdx;

    _map.add(candPtr, outParticle);
    //edm::LogWarning("fillSUEP") << candPtr.id() << "\n"; 
    if (replacedCandPtr.isNonnull())
      _map.add(replacedCandPtr, outParticle);

    for (auto* d : daughters)
      static_cast<PNodeWithPtr*>(d)->fillSUEP(_outParticles, _map, myidx);
  }

  void fillSUEP(suep::UnpackedGenParticleCollection& _outParticles, int parentIdx = -1) const {
    auto& outParticle(_outParticles.create_back());
    int myidx(_outParticles.size() - 1);

    fillP4(outParticle, *candPtr);

    outParticle.pdgid = pdgId;
    outParticle.finalState = (status == 1);
    outParticle.miniaodPacked = (miniaodPacked.find(this) != miniaodPacked.end());
    outParticle.statusFlags = statusBits.to_ulong();
    outParticle.parent.idx() = parentIdx;

    for (auto* d : daughters)
      static_cast<PNodeWithPtr*>(d)->fillSUEP(_outParticles, myidx);
  }
};

GenParticlesFiller::GenParticlesFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg),
  furtherPrune_(getParameter_<bool>(_cfg, "prune", true))
{
  getToken_(genParticlesToken_, _cfg, _coll, "common", "genParticles");
  // this is miniaod-specific
  getToken_(finalStateParticlesToken_, _cfg, _coll, "common", "finalStateParticles", false);

  if (furtherPrune_) {
    // Using a non-default function to accommodate the case where important decay trees are truncated by CMSSW pruning
    /* Removing the following:
     *  . Terminal non-final state (d.daughters.size() == 0 && d.status != 1) with pt < 0.05 GeV
     *  . Repeating link (d.daughters.size() == 1 && d.daughters[0].pdgId == d.pdgId)
     *  . Hadronic intermediates (status != 1 and (the hundreds place of |d.pdgId| is nonzero, or pdgId == 21, 81-100)) that is not the first heavy-flavor hadron in the chain
     *  . |d.pdgId| <= 3 and |d.daughters[n].pdgId| <= 3 for all n
    */
    PNode::gPruningFunction = [](PNode const& node)->bool {
      return (node.isIntermediateTerminal() && node.pt < 0.05) ||
      node.isNoDecay() ||
      (node.isHadronicIntermediate() && !node.isFirstHeavyHadron()) ||
      node.isLightDecayingToLight();
    };
  }

  unsigned outputMode(getParameter_<unsigned>(_cfg, "outputMode", 0));
  switch (outputMode) {
  case 0:
    fillPacked_ = true;
    fillUnpacked_ = false;
    break;
  case 1:
    fillPacked_ = false;
    fillUnpacked_ = true;
    break;
  case 2:
    fillPacked_ = true;
    fillUnpacked_ = true;
    break;
  default:
    throw std::runtime_error("Unknown output mode in GenParticlesFiller");
  }
}

void
GenParticlesFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList&) const
{
  if (fillPacked_)
    _eventBranches.emplace_back("genParticles");
}

void
GenParticlesFiller::addOutput(TFile& _outputFile)
{
  if (fillUnpacked_) {
    auto* eventTree(static_cast<TTree*>(_outputFile.Get("events")));
    if (!eventTree) // something is wrong
      return;

    outUnpacked.book(*eventTree);
    outputTree_ = eventTree;
  }
}

void
GenParticlesFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const&)
{
  auto& inParticles(getProduct_(_inEvent, genParticlesToken_));
  // this is miniaod-specific - modify if we need to run on AOD for some reason
  PackedGenParticleView const* inFinalStates(0);
  if (!finalStateParticlesToken_.second.isUninitialized())
    inFinalStates = &getProduct_(_inEvent, finalStateParticlesToken_);

  std::map<reco::CandidatePtr, PNodeWithPtr*> nodeMap;
  std::vector<PNodeWithPtr*> rootNodes;
  std::vector<PNodeWithPtr*> orphans;

  // A global set for tracking which PNodeWithPtr came from packed collection
  miniaodPacked.clear();

  for (unsigned iP(0); iP != inParticles.size(); ++iP) {
    auto& inCand(inParticles.at(iP));
    if (inCand.motherRefVector().size() == 0)
      rootNodes.push_back(new PNodeWithPtr(inParticles.ptrAt(iP), nodeMap));
  }
  
  if (inFinalStates) {
    for (unsigned iP(0); iP != inFinalStates->size(); ++iP) {
      auto* finalState(new PNodeWithPtr(inFinalStates->ptrAt(iP), nodeMap));
      miniaodPacked.insert(finalState);
      if (!finalState->mother)
        orphans.push_back(finalState);
    }
  }

  auto& outPacked(_outEvent.genParticles);
  if (fillUnpacked_)
    outUnpacked.init();

  // important to reserve enough space on the output collection
  // otherwise collection reallocates in the middle of fill and refs become invalid
  unsigned totalSize(inParticles.size());
  if (inFinalStates)
    totalSize += inFinalStates->size();

  if (fillPacked_)
    outPacked.reserve(totalSize);
  if (fillUnpacked_)
    outUnpacked.reserve(totalSize);
  
  auto& objectMap(objectMap_->get<reco::Candidate, suep::GenParticle>());

  for (auto* rootNode : rootNodes) {
    if (furtherPrune_)
      rootNode->pruneDaughters();

    if (fillPacked_)
      rootNode->fillSUEP(outPacked, objectMap);
    if (fillUnpacked_)
      rootNode->fillSUEP(outUnpacked);
  }

  // fill the orphans
  for (auto* orphan : orphans) {
    if (fillPacked_)
      orphan->fillSUEP(outPacked, objectMap);
    if (fillUnpacked_)
      orphan->fillSUEP(outUnpacked);
  }

  // ownDaughter is false; need to clean up pnodes
  for (auto& node : nodeMap)
    delete node.second;

  if (fillUnpacked_)
    outUnpacked.prepareFill(*outputTree_);
}

DEFINE_TREEFILLER(GenParticlesFiller);
