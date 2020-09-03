#ifndef SUEPProd_Producer_MuonsFiller_h
#define SUEPProd_Producer_MuonsFiller_h

#include "FillerBase.h"
#include "SUEPProd/Utilities/interface/RoccoR.h"

#include "DataFormats/Common/interface/View.h"
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

class MuonsFiller : public FillerBase {
 public:
  MuonsFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~MuonsFiller() {}

  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;
  void setRefs(ObjectMapStore const&) override;

 protected:
  typedef edm::View<reco::Muon> MuonView;

  NamedToken<MuonView> muonsToken_;
  NamedToken<reco::VertexCollection> verticesToken_;

  RoccoR rochesterCorrector_;
};

#endif
