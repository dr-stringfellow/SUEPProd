#include "../interface/PFCandsFiller.h"

#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/TrackReco/interface/TrackBase.h"

#include "SUEPProd/Auxiliary/interface/PackedValuesExposer.h"

PFCandsFiller::PFCandsFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg),
  useExistingWeights_(getParameter_<bool>(_cfg, "useExistingWeights", true))
{
  getToken_(candidatesToken_, _cfg, _coll, "common", "pfCandidates");
  getToken_(puppiMapToken_, _cfg, _coll, "puppiMap", false);
  getToken_(puppiInputToken_, _cfg, _coll, "puppiInput", false);
  getToken_(puppiNoLepMapToken_, _cfg, _coll, "puppiNoLepMap", false);
  getToken_(puppiNoLepInputToken_, _cfg, _coll, "puppiNoLepInput", false);
  getToken_(verticesToken_, _cfg, _coll, "common", "vertices");
}

void
PFCandsFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList&) const
{
  _eventBranches.emplace_back("pfCandidates");
  _eventBranches.emplace_back("tracks");
}

void
PFCandsFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const&)
{
  edm::Handle<reco::CandidateView> candsHandle;
  auto& inCands(getProduct_(_inEvent, candidatesToken_, &candsHandle));
  auto& inVertices(getProduct_(_inEvent, verticesToken_));

  // connect inCands and the puppi candidates by references to the base collection
  // PuppiProducer produces a ValueMap<CandidatePtr> (ref to input -> puppi candidate)
  // If the input to PuppiProducer is itself a ref collection (e.g. PtrVector), we need
  // to map the refs down to the orignal collection.
  // In more practical terms:
  //   edm::Ref<View>(viewHandle, iview) maps to a puppi candidate via puppiMap
  //   View::refAt(iview).key() is the index of the PF candidate in the original collection

  std::map<reco::CandidatePtr, reco::Candidate const*> inCandsMap;

  std::map<reco::Candidate const*, reco::CandidatePtr> puppiPtrMap;

  if (!puppiMapToken_.second.isUninitialized()) {
    for (unsigned iC(0); iC != inCands.size(); ++iC) {
      auto ptrToPF(inCands.ptrAt(iC)); // returns a pointer to the original collection (as opposed to Ref<CandidateView> ref(candsHandle, iC));
      inCandsMap[ptrToPF] = &inCands.at(iC);
    }

    auto& puppiMap(getProduct_(_inEvent, puppiMapToken_));
    edm::Handle<reco::CandidateView> puppiInputHandle;
    auto& puppiInput(getProduct_(_inEvent, puppiInputToken_, &puppiInputHandle));
    for (unsigned iC(0); iC != puppiInput.size(); ++iC) {
      edm::Ref<reco::CandidateView> inputRef(puppiInputHandle, iC);
      auto& puppiPtr(puppiMap[inputRef]);

      auto ptrToPF(puppiInput.ptrAt(iC));
      auto inCandsItr(inCandsMap.find(ptrToPF));
      if (inCandsItr == inCandsMap.end()) {
        // You are here because of a misconfiguration or because the input to puppi had some layer(s) of PF candidate cloning.
        // It may be possible to trace back to the original PF collection through calls to sourceCandidatePtr()
        // but for now we don't need to implement it.
        throw std::runtime_error("Cannot find candidate matching a puppi input");
      }

      puppiPtrMap[inCandsItr->second] = puppiPtr;
    }
  }

  std::map<reco::Candidate const*, reco::CandidatePtr> puppiNoLepPtrMap;

  if (!puppiNoLepMapToken_.second.isUninitialized()) {
    if (inCandsMap.empty()) {
      for (unsigned iC(0); iC != inCands.size(); ++iC) {
        auto ptrToPF(inCands.ptrAt(iC)); // returns a pointer to the original collection (as opposed to Ref<CandidateView> ref(candsHandle, iC));
        inCandsMap[ptrToPF] = &inCands.at(iC);
      }
    }

    auto& puppiNoLepMap(getProduct_(_inEvent, puppiNoLepMapToken_));
    edm::Handle<reco::CandidateView> puppiNoLepInputHandle;
    auto& puppiNoLepInput(getProduct_(_inEvent, puppiNoLepInputToken_, &puppiNoLepInputHandle));
    for (unsigned iC(0); iC != puppiNoLepInput.size(); ++iC) {
      edm::Ref<reco::CandidateView> inputRef(puppiNoLepInputHandle, iC);
      auto& puppiNoLepPtr(puppiNoLepMap[inputRef]);

      auto ptrToPF(puppiNoLepInput.ptrAt(iC));
      auto inCandsItr(inCandsMap.find(ptrToPF));
      if (inCandsItr == inCandsMap.end()) {
        // See above
        throw std::runtime_error("Cannot find candidate matching a puppiNoLep input");
      }

      puppiNoLepPtrMap[inCandsItr->second] = puppiNoLepPtr;
    }
  }

  auto& outCands(_outEvent.pfCandidates);

  std::vector<reco::CandidatePtr> ptrList;

  unsigned iP(-1);
  for (auto& inCand : inCands) {
    ++iP;

    auto* inPacked(dynamic_cast<pat::PackedCandidate const*>(&inCand));

    auto& outCand(outCands.create_back());

    if (inPacked) {
      // directly fill the packed values to minimize the precision loss
      PackedPatCandidateExposer exposer(*inPacked);
      outCand.packedPt = exposer.packedPt();
      outCand.packedEta = exposer.packedEta();
      outCand.packedPhi = exposer.packedPhi();
      outCand.packedM = exposer.packedM();
      if (useExistingWeights_) {
        // Except for PUPPI weights, which have changed in 10_2_4, unfortunately
        outCand.setPuppiW(inPacked->puppiWeight(), inPacked->puppiWeightNoLep());
      }

      auto vtxRef(inPacked->vertexRef());
      if (vtxRef.isNonnull())
        outCand.vertex.idx() = vtxRef.key();
      else // in reality this seems to never happen
        outCand.vertex.idx() = -1;
    }
    else {
      fillP4(outCand, inCand);
      outCand.vertex.idx() = -1;
    }

    // if puppi collection is given, use its weight
    if (!useExistingWeights_) {
      double puppiW(-1.);
      double puppiWNoLep(-1.);

      if (!puppiPtrMap.empty()) {
        auto&& ppItr(puppiPtrMap.find(&inCand));
        if (ppItr != puppiPtrMap.end() && ppItr->second.isNonnull())
          puppiW = ppItr->second->pt() / inCand.pt();
      }

      if (!puppiNoLepPtrMap.empty()) {
        auto&& ppItr(puppiNoLepPtrMap.find(&inCand));
        if (ppItr != puppiNoLepPtrMap.end() && ppItr->second.isNonnull())
          puppiWNoLep = ppItr->second->pt() / inCand.pt();
      }

      outCand.setPuppiW(puppiW, puppiWNoLep);
    }

    outCand.ptype = suep::PFCand::X;
    unsigned ptype(0);
    while (ptype != suep::PFCand::nPTypes) {
      if (suep::PFCand::pdgId_[ptype] == inCand.pdgId()) {
        outCand.ptype = ptype;
        break;
      }
      ++ptype;
    }

    outCand.hCalFrac = inPacked->hcalFraction();

    ptrList.push_back(inCands.ptrAt(iP));
  }

  auto ByVertexAndPt([](suep::Element const& e1, suep::Element const& e2)->Bool_t {
      auto& p1(static_cast<suep::PFCand const&>(e1));
      auto& p2(static_cast<suep::PFCand const&>(e2));
      if (p1.vertex.idx() == p2.vertex.idx())
        return p1.pt() > p2.pt();
      else
        return unsigned(p1.vertex.idx()) < unsigned(p2.vertex.idx());
    });

  // sort the output electrons
  auto originalIndices(outCands.sort(ByVertexAndPt));

  // make reco <-> suep mapping
  auto& objectMap(objectMap_->get<reco::Candidate, suep::PFCand>());
  auto& puppiMap(objectMap_->get<reco::Candidate, suep::PFCand>("puppi"));
  
  for (unsigned iP(0); iP != outCands.size(); ++iP) {
    auto& outCand(outCands[iP]);
    unsigned idx(originalIndices[iP]);
    auto& ptr(ptrList[idx]);
    objectMap.add(ptr, outCand);

    auto&& ppItr(puppiPtrMap.find(ptr.get()));
    if (ppItr != puppiPtrMap.end() && ppItr->second.isNonnull())
      puppiMap.add(ppItr->second, outCand);

    // add track information for charged hadrons
    // track order matters; track ref from PFCand are set during Event::getEntry relying on the order
    switch (outCand.ptype) {
    case suep::PFCand::hp:
    case suep::PFCand::hm:
    case suep::PFCand::ep:
    case suep::PFCand::em:
    case suep::PFCand::mup:
    case suep::PFCand::mum:
      {
        auto& track(_outEvent.tracks.create_back());
        PackedPatCandidateExposer exposer(static_cast<pat::PackedCandidate const&>(*ptr));

        auto* bestTrack(ptr->bestTrack());
        if (bestTrack) {
          track.setPtError(bestTrack->ptError());
          // Only highPurity is filled in miniAOD, see https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookTrackAnalysis
          track.highPurity = bestTrack->quality(reco::TrackBase::highPurity);
        }
        track.packedDxy = exposer.packedDxy();
        track.packedDz = exposer.packedDz();
        track.packedDPhi = exposer.packedDPhi();
      }
      break;
    default:
      break;
    }
  }

  outCandidates_ = &outCands;

  orderedVertices_.resize(inVertices.size());
  for (unsigned iV(0); iV != inVertices.size(); ++iV)
    orderedVertices_[iV] = inVertices.ptrAt(iV);
}

void
PFCandsFiller::setRefs(ObjectMapStore const& _objectMaps)
{
  auto& vtxMap(_objectMaps.at("vertices").get<reco::Vertex, suep::RecoVertex>().fwdMap);

  unsigned nVtx(orderedVertices_.size());

  unsigned iVtx(0);
  unsigned iPF(0);
  for (auto& cand : *outCandidates_) {
    unsigned idx(cand.vertex.idx());

    while (idx != iVtx && iVtx != nVtx) { // first candidate of the next vertex
      vtxMap.at(orderedVertices_[iVtx])->pfRangeMax = iPF;
      ++iVtx;
    }

    if (iVtx == nVtx)
      break;

    ++iPF;
  }

  while (iVtx != nVtx) {
    vtxMap.at(orderedVertices_[iVtx])->pfRangeMax = iPF;
    ++iVtx;
  }
}

DEFINE_TREEFILLER(PFCandsFiller);
