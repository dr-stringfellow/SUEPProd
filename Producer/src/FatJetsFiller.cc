#include "../interface/FatJetsFiller.h"

#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/Math/interface/deltaR.h"

#include <functional>

FatJetsFiller::FatJetsFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  JetsFiller(_name, _cfg, _coll),
  shallowBBTagTag_(getParameter_<std::string>(_cfg, "shallowBBTag", "")),
  deepBBprobQTag_(getParameter_<std::string>(_cfg, "deepBBprobQTag", "")),
  deepBBprobHTag_(getParameter_<std::string>(_cfg, "deepBBprobHTag", "")),
  njettinessTag_(getParameter_<std::string>(_cfg, "njettiness")),
  sdKinematicsTag_(getParameter_<std::string>(_cfg, "sdKinematics")),
  prunedKinematicsTag_(getParameter_<std::string>(_cfg, "prunedKinematics")),
  subjetBtagTag_(getParameter_<std::string>(_cfg, "subjetBtag", "")),
  subjetQGLTag_(getParameter_<std::string>(_cfg, "subjetQGL", "")),
  subjetCmvaTag_(getParameter_<std::string>(_cfg, "subjetCmva", "")),
  subjetDeepCsvTag_(getParameter_<std::string>(_cfg, "subjetDeepCSV", "")),
  subjetDeepCmvaTag_(getParameter_<std::string>(_cfg, "subjetDeepCMVA", "")),
  activeArea_(7., 1, 0.01),
  areaDef_(fastjet::active_area_explicit_ghosts, activeArea_)
{
  if (_name == "puppiAK8Jets")
    outSubjetSelector_ = [](suep::Event& _event)->suep::MicroJetCollection& { return _event.puppiAK8Subjets; };
  else if (_name == "puppiCA15Jets")
    outSubjetSelector_ = [](suep::Event& _event)->suep::MicroJetCollection& { return _event.puppiCA15Subjets; };
  else
    throw edm::Exception(edm::errors::Configuration, "Unknown JetCollection output");

  getToken_(subjetsToken_, _cfg, _coll, "subjets");

  auto&& computeMode(getParameter_<std::string>(_cfg, "computeSubstructure", ""));
  if (computeMode == "always")
    computeSubstructure_ = kAlways;
  else if (computeMode == "recoil")
    computeSubstructure_ = kLargeRecoil;
  else
    computeSubstructure_ = kNever;

  if (computeSubstructure_ == kLargeRecoil)
    getToken_(categoriesToken_, _cfg, _coll, "recoil");

  if (computeSubstructure_ != kNever) {
    jetDefCA_ = new fastjet::JetDefinition(fastjet::cambridge_algorithm, R_);
    softdrop_ = new fastjet::contrib::SoftDrop(1., 0.15, R_);
    ecfcalc_ = new suepecf::Calculator();
    tau_ = new fastjet::contrib::Njettiness(fastjet::contrib::OnePass_KT_Axes(), fastjet::contrib::NormalizedMeasure(1., R_));

    //htt
    bool optimalR=true; bool doHTTQ=false;
    double minSJPt=0.; double minCandPt=0.;
    double sjmass=30.; double mucut=0.8;
    double filtR=0.3; int filtN=5;
    int mode=4; double minCandMass=0.;
    double maxCandMass=9999999.; double massRatioWidth=9999999.;
    double minM23Cut=0.; double minM13Cut=0.;
    double maxM13Cut=9999999.;  bool rejectMinR=false;
    htt_ = new fastjet::HEPTopTaggerV2(optimalR,doHTTQ,
                                        minSJPt,minCandPt,
                                        sjmass,mucut,
                                        filtR,filtN,
                                        mode,minCandMass,
                                        maxCandMass,massRatioWidth,
                                        minM23Cut,minM13Cut,
                                        maxM13Cut,rejectMinR);
  }
}

FatJetsFiller::~FatJetsFiller()
{
  delete jetDefCA_;
  delete softdrop_;
  delete ecfcalc_;
  delete tau_;
  delete htt_;
}

void
FatJetsFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList& _runBranches) const
{
  JetsFiller::branchNames(_eventBranches, _runBranches);

  _eventBranches.emplace_back("!" + getName() + ".area");

  TString subjetName(getName());
  subjetName.ReplaceAll("Jets", "Subjets");
  _eventBranches.emplace_back(subjetName);

  if (computeSubstructure_ == kNever) {
    char const* substrBranches[] = {
      ".tau1SD",
      ".tau2SD",
      ".tau3SD",
      ".htt_mass",
      ".htt_frec",
      ".ecfs"
    };
    for (char const* b : substrBranches)
      _eventBranches.emplace_back("!" + getName() + b);
  }
}

void
FatJetsFiller::fillDetails_(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const& _setup)
{
  bool doSubstructure(computeSubstructure_ == kAlways ||
                      (computeSubstructure_ == kLargeRecoil && getProduct_(_inEvent, categoriesToken_) != 0));

  auto& inSubjets(getProduct_(_inEvent, subjetsToken_));

  auto& outSubjets(outSubjetSelector_(_outEvent));

  typedef std::vector<fastjet::PseudoJet> VPseudoJet;

  auto& jetMap(objectMap_->get<reco::Jet, suep::Jet>());

  unsigned iJ(0);

  for (auto& link : jetMap.bwdMap) { // suep -> edm
    auto& outJet(static_cast<suep::FatJet&>(*link.first));

    if (dynamic_cast<pat::Jet const*>(link.second.get())) {
      auto& inJet(static_cast<pat::Jet const&>(*link.second));

      outJet.tau1 = inJet.userFloat(njettinessTag_ + ":tau1");
      outJet.tau2 = inJet.userFloat(njettinessTag_ + ":tau2");
      outJet.tau3 = inJet.userFloat(njettinessTag_ + ":tau3");
      outJet.mSD  = inJet.userFloat(sdKinematicsTag_ + ":Mass");
      outJet.mPruned = inJet.userFloat(prunedKinematicsTag_ + ":Mass");

      if (!shallowBBTagTag_.empty())
        outJet.double_sub = inJet.bDiscriminator(shallowBBTagTag_);
      if (!deepBBprobQTag_.empty())
        outJet.deepBBprobQ = inJet.bDiscriminator(deepBBprobQTag_);
      if (!deepBBprobHTag_.empty())
        outJet.deepBBprobH = inJet.bDiscriminator(deepBBprobHTag_);

      for (auto& inSubjet : inSubjets) {
        if (reco::deltaR(inSubjet.eta(), inSubjet.phi(), inJet.eta(), inJet.phi()) > R_) 
          continue;

        auto& outSubjet(outSubjets.create_back());

        fillP4(outSubjet, inSubjet);

        if (dynamic_cast<pat::Jet const*>(&inSubjet)) {
          auto& patSubjet(dynamic_cast<pat::Jet const&>(inSubjet));
          if (!subjetBtagTag_.empty())
            outSubjet.csv = patSubjet.bDiscriminator(subjetBtagTag_);
          if (!subjetCmvaTag_.empty())
            outSubjet.cmva = patSubjet.bDiscriminator(subjetCmvaTag_);
          if (!subjetQGLTag_.empty() && patSubjet.hasUserFloat(subjetQGLTag_))
            outSubjet.qgl = patSubjet.userFloat(subjetQGLTag_);

          if (!subjetDeepCsvTag_.empty()) {
            for (auto prob : deepProbs) {
              fillDeepBySwitch_(outSubjet, prob.second, patSubjet.bDiscriminator(subjetDeepCsvTag_ + ":prob" + prob.first));
            }
          }

          if (!subjetDeepCmvaTag_.empty()) {
            for (auto prob : deepProbs) {
              fillDeepBySwitch_(outSubjet, prob.second + deepSuff::DEEP_SIZE, patSubjet.bDiscriminator(subjetDeepCmvaTag_ + ":prob" + prob.first));
            }
          }

        }

        outJet.subjets.addRef(&outSubjet);
      }

      // reset the ECFs
      for (unsigned iB(0); iB != 4; ++iB) {
        for (int N : {1, 2, 3, 4}) {
          for (int order : {1, 2, 3}) {
            outJet.set_ecf(order, N, iB, -1);
          }
        }
      }

      if (doSubstructure && iJ < 2) {
        // either we want to associate to pf cands OR compute extra info about the first or second jet
        // but do not do any of this if ReduceEvent() is tripped
        // only filled for first two fat jets

        // calculate ECFs, groomed tauN
        VPseudoJet vjet;
        for (auto&& ptr : inJet.getJetConstituents()) { 
          // create vector of PseudoJets
          auto& cand(*ptr);
          if (cand.pt() < 0.01) 
            continue;

          vjet.emplace_back(cand.px(), cand.py(), cand.pz(), cand.energy());
        }

        fastjet::ClusterSequenceArea seq(vjet, *jetDefCA_, areaDef_);
        VPseudoJet alljets(fastjet::sorted_by_pt(seq.inclusive_jets(0.1)));

        if (alljets.size() > 0){
          fastjet::PseudoJet& leadingJet(alljets[0]);
          fastjet::PseudoJet sdJet((*softdrop_)(leadingJet));

          // get and filter constituents of groomed jet
          VPseudoJet sdconsts(fastjet::sorted_by_pt(sdJet.constituents()));
          unsigned nFilter(std::min(100, int(sdconsts.size())));
          VPseudoJet sdconstsFiltered(sdconsts.begin(), sdconsts.begin() + nFilter);

          // calculate ECFs
          ecfcalc_->calculate(sdconstsFiltered);
          for (auto iter = ecfcalc_->begin(); iter != ecfcalc_->end(); ++iter) {
            int oI = iter.get<suepecf::Calculator::oP>() + 1;
            int nI = iter.get<suepecf::Calculator::nP>() + 1;
            int bI = iter.get<suepecf::Calculator::bP>();
            bool success = outJet.set_ecf(oI, nI, bI,
                                          static_cast<float>(iter.get<suepecf::Calculator::ecfP>()));
            if (!success)
              throw std::runtime_error(
                  TString::Format("FatJetsFiller Could not save oI=%i, nI=%i, bI=%i", oI, nI, bI).Data());
          }

          outJet.tau3SD = tau_->getTau(3, sdconsts);
          outJet.tau2SD = tau_->getTau(2, sdconsts);
          outJet.tau1SD = tau_->getTau(1, sdconsts);

          // HTT
          fastjet::PseudoJet httJet(htt_->result(leadingJet));
          if (httJet != 0) {
            auto* s(static_cast<fastjet::HEPTopTaggerV2Structure*>(httJet.structure_non_const_ptr()));
            outJet.htt_mass = s->top_mass();
            outJet.htt_frec = s->fRec();
          }
        }
        else
          throw std::runtime_error("SUEPProd::FatJetsFiller: Jet could not be clustered");
      } // if computeSubstructure_
    }

    ++iJ;
  }
}

DEFINE_TREEFILLER(FatJetsFiller);
