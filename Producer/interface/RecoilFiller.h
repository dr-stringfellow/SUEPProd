#ifndef SUEPProd_Producer_RecoilFiller_h
#define SUEPProd_Producer_RecoilFiller_h

#include "FillerBase.h"

class RecoilFiller : public FillerBase {
 public:
  RecoilFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~RecoilFiller() {}

  void addOutput(TFile&) override;
  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;

 protected:
  NamedToken<int> categoriesToken_;
  NamedToken<double> maxToken_;
};

#endif
