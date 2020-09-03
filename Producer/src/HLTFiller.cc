#include "../interface/HLTFiller.h"

#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/PatCandidates/interface/MET.h"

HLTFiller::HLTFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg)
{
  getToken_(triggerResultsToken_, _cfg, _coll, "triggerResults");
  // Trigger object collection name was different in 2017A PromptReco
  // Using notifyNewProduct() to dynamically find the tag
  triggerObjectsToken_.first = "triggerObjects";
}

HLTFiller::~HLTFiller()
{
}

void
HLTFiller::addOutput(TFile& _outputFile)
{
  TDirectory::TContext context(&_outputFile);
  hltTree_ = new TTree("hlt", "HLT");
}

void
HLTFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList& _runBranches) const
{
  _eventBranches.emplace_back("triggers");
  _eventBranches.emplace_back("triggerObjects");
  _runBranches.emplace_back("hltMenu");
}

void
HLTFiller::fillBeginRun(suep::Run& _outRun, edm::Run const& _inRun, edm::EventSetup const& _setup)
{
  bool configChanged(false);
  if (!hltConfig_.init(_inRun, _setup, "HLT", configChanged)) {
    throw edm::Exception(edm::errors::Configuration, "HLTFiller")
      << "Cannot access hlt config";
  }

  TString menu(hltConfig_.tableName());

  // _outRun.hlt is not reset at each init() call, but _outRun itself is reset.
  // In particular _outRun.hltMenu is set to 0 at each beginRun(), which means
  // we must load the current menu from menuMap + hltTree even when the configuration
  // has not changed.
  if (!configChanged && menu != *_outRun.hlt.menu)
    throw edm::Exception(edm::errors::Configuration, "HLTFiller")
      << "HLTConfigProvider claims nothing is changed, but the menu name did.";

  *_outRun.hlt.menu = menu;
  auto menuItr(menuMap_.find(menu));

  filters_ = _outRun.hlt.filters; // just need to do this once

  filterIndices_.clear();

  if (menuItr != menuMap_.end()) {
    // We have processed this menu before. Loading it from the tree.

    _outRun.hltMenu = menuItr->second;
    hltTree_->GetEntry(_outRun.hltMenu);
    
    unsigned iF(0);
    for (TString& filter : *_outRun.hlt.filters)
      filterIndices_.emplace(filter.Data(), iF++);

    return;
  }

  // First encounter with the menu. Increment the menuMap size.

  _outRun.hltMenu = menuMap_.size();
  menuMap_.emplace(menu, _outRun.hltMenu);

  _outRun.hlt.paths->clear();
  _outRun.hlt.filters->clear();

  for (unsigned iP(0); iP != hltConfig_.size(); ++iP) {
    _outRun.hlt.paths->push_back(hltConfig_.triggerName(iP));

    for (std::string filter : hltConfig_.saveTagsModules(iP)) {
      if (filter[0] == '-' || filter[0] == ' ')
        filter = filter.substr(1);

      if (filterIndices_.count(filter) == 0) {
        filterIndices_.emplace(filter, _outRun.hlt.filters->size());
        _outRun.hlt.filters->emplace_back(filter);
      }
    }
  }

  hltTree_->Fill();
}

void
HLTFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const& _setup)
{
  auto& inTriggerResults(getProduct_(_inEvent, triggerResultsToken_));
  auto& inTriggerObjects(getProduct_(_inEvent, triggerObjectsToken_));

  auto& outHLT(_outEvent.triggers);
  auto& outObjects(_outEvent.triggerObjects);

  for (unsigned iF(0); iF != inTriggerResults.size(); ++iF) {
    if (inTriggerResults.accept(iF))
      outHLT.set(iF);
  }

  auto& objMap(objectMap_->get<pat::TriggerObjectStandAlone, suep::HLTObject>());
  // This is used in trigger object matching
  auto& nameMap(objectMap_->get<pat::TriggerObjectStandAlone, VString>());

  // Resize first so that the pointers don't become in the loop
  filterNames_.resize(inTriggerObjects.size());

  outObjects.reserve(inTriggerObjects.size());

  unsigned iObj(-1);
  for (auto inObj : inTriggerObjects) { // cloning input objects to unpack
    ++iObj;
    auto& outObj(outObjects.create_back());

    fillP4(outObj, inObj);

    inObj.unpackFilterLabels(_inEvent, inTriggerResults);

    filterNames_[iObj].clear();

    for (auto& label : inObj.filterLabels()) {
      auto itr(filterIndices_.find(label));
      if (itr != filterIndices_.end())
        outObj.filters->push_back(itr->second);

      filterNames_[iObj].push_back(label);
    }

    auto ptr(inTriggerObjects.ptrAt(iObj));
    objMap.add(ptr, outObj);
    nameMap.add(ptr, filterNames_[iObj]);
  }
}

void
HLTFiller::notifyNewProduct(edm::BranchDescription const& _bdesc, edm::ConsumesCollector& _coll)
{
  if (_bdesc.unwrappedTypeID() == edm::TypeID(typeid(std::vector<pat::TriggerObjectStandAlone>))) {
    edm::InputTag tag(_bdesc.moduleLabel(), _bdesc.productInstanceName(), _bdesc.processName());
    triggerObjectsToken_.second = _coll.consumes<TriggerObjectView>(tag);
  }
}

DEFINE_TREEFILLER(HLTFiller);
