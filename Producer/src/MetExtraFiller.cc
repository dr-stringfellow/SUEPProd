#include "../interface/MetExtraFiller.h"

#include "DataFormats/METReco/interface/GenMET.h"

TString metType[MetExtraFiller::nMetTypes] = {
  "raw",
  "calo",
  "noMu",
  "noHF",
  "trk",
  "neutral",
  "photon",
  "hf",
  "gen"
};

MetExtraFiller::MetExtraFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg)
{
  auto&& types(getParameter_<std::vector<std::string>>(_cfg, "types"));
  std::fill_n(enabled_, nMetTypes, false);
  for (auto& type : types) {
    for (unsigned iM(0); iM != nMetTypes; ++iM) {
      if (type == metType[iM]) {
        enabled_[iM] = true;
      }
    }
  }

  getToken_(patMetToken_, _cfg, _coll, "patMet", false);
  getToken_(noHFMetToken_, _cfg, _coll, "noHFMet", false);
  getToken_(genMetToken_, _cfg, _coll, "genMet", false);
  getToken_(candidatesToken_, _cfg, _coll, "common", "pfCandidates", false);

  if ((enabled_[kRaw] || enabled_[kCalo] || enabled_[kNoMu]) && patMetToken_.second.isUninitialized())
    throw edm::Exception(edm::errors::Configuration, "patMet is required for raw, calo, and noMu METs");
  if (enabled_[kNoHF] && noHFMetToken_.second.isUninitialized())
    throw edm::Exception(edm::errors::Configuration, "noHFMet is required for noHF MET");
  if (enabled_[kGen] && patMetToken_.second.isUninitialized() && genMetToken_.second.isUninitialized())
    throw edm::Exception(edm::errors::Configuration, "patMet or genMet is required for gen MET");
  if ((enabled_[kNoMu] || enabled_[kTrk] || enabled_[kNeutral] || enabled_[kPhoton] || enabled_[kHF]) && candidatesToken_.second.isUninitialized())
    throw edm::Exception(edm::errors::Configuration, "pfCandidates is required for nomu, trk, neutral, photon, and hf METs");
}

void
MetExtraFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList&) const
{
  for (unsigned iM(0); iM != nMetTypes; ++iM) {
    if (enabled_[iM])
      _eventBranches.push_back(metType[iM] + "Met");
  }
}

void
MetExtraFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const& _setup)
{
  auto* patMets(getProductSafe_(_inEvent, patMetToken_));
  auto* noHFMets(getProductSafe_(_inEvent, noHFMetToken_));
  auto* genMets(getProductSafe_(_inEvent, genMetToken_));
  auto* candidates(getProductSafe_(_inEvent, candidatesToken_));

  double noMuMex(0.);
  double noMuMey(0.);

  if (patMets) {
    auto& patMet(patMets->at(0));

    if (enabled_[kRaw]) {
      _outEvent.rawMet.pt = patMet.uncorPt();
      _outEvent.rawMet.phi = patMet.uncorPhi();
    }

    if (enabled_[kCalo]) {
      _outEvent.caloMet.pt = patMet.caloMETPt();
      _outEvent.caloMet.phi = patMet.caloMETPhi();
    }

    if (enabled_[kGen]) {
      _outEvent.genMet.pt = patMet.genMET()->pt();
      _outEvent.genMet.phi = patMet.genMET()->phi();
    }

    noMuMex = patMet.px();
    noMuMey = patMet.py();
  }
  else if (genMets && enabled_[kGen]) {
    auto& genMet(genMets->at(0));

    _outEvent.genMet.pt = genMet.pt();
    _outEvent.genMet.phi = genMet.phi();
  }

  if (noHFMets && enabled_[kNoHF]) {
    auto& noHFMet(noHFMets->at(0));

    _outEvent.noHFMet.pt = noHFMet.pt();
    _outEvent.noHFMet.phi = noHFMet.phi();
  }

  if (candidates) {
    double trkMex(0.);
    double trkMey(0.);
    double neutralMex(0.);
    double neutralMey(0.);
    double photonMex(0.);
    double photonMey(0.);
    double hfMex(0.);
    double hfMey(0.);

    for (auto& cand : *candidates) {
      if (std::abs(cand.pdgId()) == 13) {
        noMuMex += cand.px();
        noMuMey += cand.py();
      }
      else if (cand.pdgId() == 130) {
        neutralMex -= cand.px();
        neutralMey -= cand.py();
      }
      else if (cand.pdgId() == 22) {
        photonMex -= cand.px();
        photonMey -= cand.py();
      }
      else if (cand.pdgId() == 1 || cand.pdgId() == 2) {
        hfMex -= cand.px();
        hfMey -= cand.py();
      }

      if (cand.charge() != 0) {
        trkMex -= cand.px();
        trkMey -= cand.py();
      }
    }

    if (enabled_[kNoMu])
      _outEvent.noMuMet.setXY(noMuMex, noMuMey);
    if (enabled_[kTrk])
      _outEvent.trkMet.setXY(trkMex, trkMey);
    if (enabled_[kNeutral])
      _outEvent.neutralMet.setXY(neutralMex, neutralMey);
    if (enabled_[kPhoton])
      _outEvent.photonMet.setXY(photonMex, photonMey);
    if (enabled_[kHF])
      _outEvent.hfMet.setXY(hfMex, hfMey);
  }
}

DEFINE_TREEFILLER(MetExtraFiller);
