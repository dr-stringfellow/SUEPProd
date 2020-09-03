#ifndef SUEPProd_Producer_MetExtraFiller_h
#define SUEPProd_Producer_MetExtraFiller_h

#include "FillerBase.h"

#include "DataFormats/Common/interface/View.h"
#include "DataFormats/Candidate/interface/CandidateFwd.h"
#include "DataFormats/METReco/interface/MET.h"
#include "DataFormats/METReco/interface/GenMETFwd.h"
#include "DataFormats/PatCandidates/interface/MET.h"

class MetExtraFiller : public FillerBase {
 public:
  MetExtraFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~MetExtraFiller() {}

  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;

  enum MetType {
    kRaw,
    kCalo,
    kNoMu,
    kNoHF,
    kTrk,
    kNeutral,
    kPhoton,
    kHF,
    kGen,
    nMetTypes
  };

 protected:
  typedef edm::View<reco::MET> METView;

  NamedToken<pat::METCollection> patMetToken_;
  NamedToken<METView> noHFMetToken_;
  NamedToken<reco::GenMETCollection> genMetToken_;
  NamedToken<reco::CandidateView> candidatesToken_;

  bool enabled_[nMetTypes];
};

#endif
