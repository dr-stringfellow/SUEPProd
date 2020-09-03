#ifndef SUEPProd_Producer_HLTFiller_h
#define SUEPProd_Producer_HLTFiller_h

#include "FillerBase.h"

#include "DataFormats/Common/interface/TriggerResults.h"
#include "DataFormats/PatCandidates/interface/TriggerObjectStandAlone.h"
#include "HLTrigger/HLTcore/interface/HLTConfigProvider.h"

class HLTFiller : public FillerBase {
 public:
  HLTFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~HLTFiller();

  void addOutput(TFile&) override;
  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;
  void fillBeginRun(suep::Run&, edm::Run const&, edm::EventSetup const&) override;
  void notifyNewProduct(edm::BranchDescription const&, edm::ConsumesCollector&) override;

 protected:
  typedef edm::View<pat::TriggerObjectStandAlone> TriggerObjectView;

  NamedToken<edm::TriggerResults> triggerResultsToken_;
  NamedToken<TriggerObjectView> triggerObjectsToken_;

  HLTConfigProvider hltConfig_;
  std::map<TString, unsigned> menuMap_{};

  TTree* hltTree_{0};
  std::vector<TString>* filters_;

  // Map of filter name to the index in the stored filters vector
  std::map<std::string, unsigned> filterIndices_;

  // This filler exports a map of trigger object -> list of associated HLT filters
  // In CMSSW 9 series, filter names are packed and cannot be accessed from the trigger object
  // without passing an Event and TriggerResults object.
  // Since fillers with trigger matching will not have access to these information in their
  // setRef() functions, this is the only solution.
  // The vector needs to be a member data of this class to ensure validity of the pointer in
  // the objectMaps.
  std::vector<VString> filterNames_;
};

#endif
