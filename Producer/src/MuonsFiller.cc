#include "../interface/MuonsFiller.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/RandomNumberGenerator.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/Common/interface/RefToPtr.h"

#include "CLHEP/Random/RandomEngine.h"
#include "CLHEP/Random/RandFlat.h"

MuonsFiller::MuonsFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg),
  rochesterCorrector_(edm::FileInPath(getParameter_<std::string>(_cfg, "rochesterCorrectionSource")).fullPath())
{
  getToken_(muonsToken_, _cfg, _coll, "muons");
  getToken_(verticesToken_, _cfg, _coll, "common", "vertices");
}

void
MuonsFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList&) const
{
  _eventBranches.emplace_back("muons");

  if (isRealData_)
    _eventBranches.emplace_back("!muons.matchedGen_");
}

void
MuonsFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const& _setup)
{
  auto& inMuons(getProduct_(_inEvent, muonsToken_));
  auto& vertices(getProduct_(_inEvent, verticesToken_));

  auto& outMuons(_outEvent.muons);

  CLHEP::RandFlat* random(0);

  if (!isRealData_) {
    // random number used for Rochester corrections
    random = new CLHEP::RandFlat(edm::Service<edm::RandomNumberGenerator>()->getEngine(_inEvent.streamID()));
  }

  std::vector<edm::Ptr<reco::Muon>> ptrList;

  unsigned iMu(-1);
  for (auto& inMuon : inMuons) {
    ++iMu;
    auto* patMuon(dynamic_cast<pat::Muon const*>(&inMuon));

    auto& outMuon(outMuons.create_back());

    fillP4(outMuon, inMuon);

    outMuon.global = inMuon.isGlobalMuon();
    outMuon.tracker = inMuon.isTrackerMuon();
    outMuon.pf = inMuon.isPFMuon();
    outMuon.standalone = inMuon.isStandAloneMuon();
    outMuon.calo = inMuon.isCaloMuon();
    outMuon.rpc = inMuon.isRPCMuon();
    outMuon.gem = inMuon.isGEMMuon();
    outMuon.me0 = inMuon.isME0Muon();
    
    auto&& innerTrack(inMuon.innerTrack());
    if (innerTrack.isNonnull()) {
      outMuon.validFraction = innerTrack->validFraction();
      auto&& hitPattern(innerTrack->hitPattern());
      outMuon.trkLayersWithMmt = hitPattern.trackerLayersWithMeasurement();
      outMuon.pixLayersWithMmt = hitPattern.pixelLayersWithMeasurement();
      outMuon.nValidPixel = hitPattern.numberOfValidPixelHits();
    }

    auto&& globalTrack(inMuon.globalTrack());
    if (globalTrack.isNonnull()) {
      outMuon.normChi2 = globalTrack->normalizedChi2();
      auto&& hitPattern(globalTrack->hitPattern());
      outMuon.nValidMuon = hitPattern.numberOfValidMuonHits();
    }

    outMuon.nMatched = inMuon.numberOfMatchedStations();

    auto&& combQuality(inMuon.combinedQuality());
    outMuon.chi2LocalPosition = combQuality.chi2LocalPosition;
    outMuon.trkKink = combQuality.trkKink;

    outMuon.segmentCompatibility = muon::segmentCompatibility(inMuon);

    outMuon.charge = inMuon.charge();

    auto& pfIso(inMuon.pfIsolationR04());

    outMuon.chIso = pfIso.sumChargedHadronPt;
    outMuon.nhIso = pfIso.sumNeutralHadronEt;
    outMuon.phIso = pfIso.sumPhotonEt;
    outMuon.puIso = pfIso.sumPUPt;
    outMuon.r03Iso = inMuon.isolationR03().sumPt;

    outMuon.loose = inMuon.passed(reco::Muon::CutBasedIdLoose);
    outMuon.medium = inMuon.passed(reco::Muon::CutBasedIdMedium);
    outMuon.mediumPrompt = inMuon.passed(reco::Muon::CutBasedIdMediumPrompt);
    outMuon.tight = inMuon.passed(reco::Muon::CutBasedIdTight);
    outMuon.globalHighPt = inMuon.passed(reco::Muon::CutBasedIdGlobalHighPt);
    outMuon.trkHighPt = inMuon.passed(reco::Muon::CutBasedIdTrkHighPt);
    outMuon.soft = inMuon.passed(reco::Muon::SoftCutBasedId);
    outMuon.softMVA = inMuon.passed(reco::Muon::SoftMvaId);
    outMuon.mvaLoose = inMuon.passed(reco::Muon::MvaLoose);
    outMuon.mvaMedium = inMuon.passed(reco::Muon::MvaMedium);
    outMuon.mvaTight = inMuon.passed(reco::Muon::MvaTight);
    outMuon.pfIsoVeryLoose = inMuon.passed(reco::Muon::PFIsoVeryLoose);
    outMuon.pfIsoLoose = inMuon.passed(reco::Muon::PFIsoLoose);
    outMuon.pfIsoMedium = inMuon.passed(reco::Muon::PFIsoMedium);
    outMuon.pfIsoTight = inMuon.passed(reco::Muon::PFIsoTight);
    outMuon.pfIsoVeryTight = inMuon.passed(reco::Muon::PFIsoVeryTight);
    outMuon.tkIsoLoose = inMuon.passed(reco::Muon::TkIsoLoose);
    outMuon.tkIsoTight = inMuon.passed(reco::Muon::TkIsoTight);
    outMuon.miniIsoLoose = inMuon.passed(reco::Muon::MiniIsoLoose);
    outMuon.miniIsoMedium = inMuon.passed(reco::Muon::MiniIsoMedium);
    outMuon.miniIsoTight = inMuon.passed(reco::Muon::MiniIsoTight);
    outMuon.miniIsoVeryTight = inMuon.passed(reco::Muon::MiniIsoVeryTight);

    outMuon.hltsafe = outMuon.combIso() / outMuon.pt() < 0.4 && outMuon.r03Iso / outMuon.pt() < 0.4;

    auto bestTrack(inMuon.muonBestTrack());
    if (vertices.size() != 0) {
      auto& pv(vertices.at(0));
      auto pos(pv.position());
      if (patMuon)
        outMuon.dxy = patMuon->dB(); // probably gives identical value as bestTrack->dxy()
      else
        outMuon.dxy = std::abs(bestTrack->dxy(pos));

      outMuon.dz = std::abs(bestTrack->dz(pos));
    }
    else {
      if (patMuon)
        outMuon.dxy = patMuon->dB(); // probably gives identical value as bestTrack->dxy()
      else
        outMuon.dxy = std::abs(bestTrack->dxy());

      outMuon.dz = std::abs(bestTrack->dz());
    }

    outMuon.pfPt = inMuon.pfP4().pt();

    // Rochester correction
    // See SUEPProd/Utilities/doc/README.RoccoR
    if (isRealData_) {
      outMuon.rochCorr = rochesterCorrector_.kScaleDT(outMuon.charge, outMuon.pt(), outMuon.eta(), outMuon.phi());
      outMuon.rochCorrErr = rochesterCorrector_.kScaleDTerror(outMuon.charge, outMuon.pt(), outMuon.eta(), outMuon.phi());
    }
    else {
      double u1(random->fire());
      try {
        if (patMuon && patMuon->genParticleRef().isNonnull()) {
          auto& gen(*patMuon->genParticleRef());
          outMuon.rochCorr = rochesterCorrector_.kScaleFromGenMC(outMuon.charge, outMuon.pt(), outMuon.eta(), outMuon.phi(), outMuon.trkLayersWithMmt, gen.pt(), u1);
          outMuon.rochCorrErr = rochesterCorrector_.kScaleFromGenMCerror(outMuon.charge, outMuon.pt(), outMuon.eta(), outMuon.phi(), outMuon.trkLayersWithMmt, gen.pt(), u1);
        }
        else {
          double u2(random->fire());
          outMuon.rochCorr = rochesterCorrector_.kScaleAndSmearMC(outMuon.charge, outMuon.pt(), outMuon.eta(), outMuon.phi(), outMuon.trkLayersWithMmt, u1, u2);
          outMuon.rochCorrErr = rochesterCorrector_.kScaleAndSmearMCerror(outMuon.charge, outMuon.pt(), outMuon.eta(), outMuon.phi(), outMuon.trkLayersWithMmt, u1, u2);
        }
      }
      catch (std::exception& ex) {
        // Rochester correction can throw or be nan for certain combination of parameters
        outMuon.rochCorr = -1.;
        outMuon.rochCorrErr = 0.;
      }
    }

    ptrList.push_back(inMuons.ptrAt(iMu));
  }
  
  auto originalIndices(outMuons.sort(suep::Particle::PtGreater));

  // export suep <-> reco mapping

  auto& muMuMap(objectMap_->get<reco::Muon, suep::Muon>());
  auto& pfMuMap(objectMap_->get<reco::Candidate, suep::Muon>("pf"));
  auto& vtxMuMap(objectMap_->get<reco::Vertex, suep::Muon>());
  auto& genMuMap(objectMap_->get<reco::Candidate, suep::Muon>("gen"));

  for (unsigned iP(0); iP != outMuons.size(); ++iP) {
    auto& outMuon(outMuons[iP]);
    unsigned idx(originalIndices[iP]);
    muMuMap.add(ptrList[idx], outMuon);

    auto sourcePtr(ptrList[idx]->sourceCandidatePtr(0));
    if (sourcePtr.isNonnull()) {
      pfMuMap.add(sourcePtr, outMuon);
      if (dynamic_cast<pat::PackedCandidate const*>(sourcePtr.get())) {
        auto vtxRef(static_cast<pat::PackedCandidate const&>(*sourcePtr).vertexRef());
        if (vtxRef.isNonnull())
          vtxMuMap.add(edm::refToPtr(vtxRef), outMuon);
      }
    }

    if (!isRealData_) {
      auto& inMuon(*ptrList[idx]);

      if (dynamic_cast<pat::Muon const*>(&inMuon)) {
        auto& patMuon(static_cast<pat::Muon const&>(inMuon));
        auto ref(patMuon.genParticleRef());
        if (ref.isNonnull())
          genMuMap.add(edm::refToPtr(ref), outMuon);
      }
    }
  }
}

void
MuonsFiller::setRefs(ObjectMapStore const& _objectMaps)
{
  auto& pfMuMap(objectMap_->get<reco::Candidate, suep::Muon>("pf"));
  auto& vtxMuMap(objectMap_->get<reco::Vertex, suep::Muon>());

  auto& pfMap(_objectMaps.at("pfCandidates").get<reco::Candidate, suep::PFCand>().fwdMap);
  auto& vtxMap(_objectMaps.at("vertices").get<reco::Vertex, suep::RecoVertex>().fwdMap);

  for (auto& link : pfMuMap.bwdMap) { // suep -> edm
    auto& outMuon(*link.first);
    auto& pfPtr(link.second);

    // muon sourceCandidatePtr can point to the AOD pfCandidates in some cases
    auto pfItr(pfMap.find(pfPtr));
    if (pfItr == pfMap.end())
      continue;

    outMuon.matchedPF.setRef(pfItr->second);
  }

  for (auto& link : vtxMuMap.bwdMap) { // suep -> edm
    auto& outMuon(*link.first);
    auto& vtxPtr(link.second);

    outMuon.vertex.setRef(vtxMap.at(vtxPtr));
  }

  if (!isRealData_) {
    auto& genMuMap(objectMap_->get<reco::Candidate, suep::Muon>("gen"));

    auto& genMap(_objectMaps.at("genParticles").get<reco::Candidate, suep::GenParticle>().fwdMap);

    for (auto& link : genMuMap.bwdMap) {
      auto& genPtr(link.second);
      if (genMap.find(genPtr) == genMap.end())
        continue;

      auto& outMuon(*link.first);
      outMuon.matchedGen.setRef(genMap.at(genPtr));
    }
  }
}

DEFINE_TREEFILLER(MuonsFiller);
