#ifndef SUEPProd_Producer_PartonsFiller_h
#define SUEPProd_Producer_PartonsFiller_h

#include "FillerBase.h"

#include "SimDataFormats/GeneratorProducts/interface/LHEEventProduct.h"

class PartonsFiller : public FillerBase {
 public:
  PartonsFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~PartonsFiller() {}

  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;
  void notifyNewProduct(edm::BranchDescription const&, edm::ConsumesCollector&) override;

 protected:
  NamedToken<LHEEventProduct> lheEventToken_;
};

#endif
