#ifndef SUEPProd_Producer_MetFiltersFiller_h
#define SUEPProd_Producer_MetFiltersFiller_h

#include "FillerBase.h"

#include "DataFormats/Common/interface/TriggerResults.h"
#include "DataFormats/DetId/interface/DetIdCollection.h"

class MetFiltersFiller : public FillerBase {
 public:
  MetFiltersFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~MetFiltersFiller() {}

  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;

 protected:
  std::vector<NamedToken<edm::TriggerResults>> filterResultsTokens_;
};

#endif
