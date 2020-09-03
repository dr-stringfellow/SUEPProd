#include "../interface/MetFiltersFiller.h"

#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/PatCandidates/interface/MET.h"

MetFiltersFiller::MetFiltersFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg)
{
  for (auto& proc : getParameter_<VString>(_cfg, "filterProcesses"))
    filterResultsTokens_.emplace_back("TriggerResults:" + proc, _coll.consumes<edm::TriggerResults>(edm::InputTag("TriggerResults", "", proc)));
}

void
MetFiltersFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList&) const
{
  _eventBranches.emplace_back("metFilters");
}

void
MetFiltersFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const& _setup)
{
  auto& outMetFilters(_outEvent.metFilters);

  for (auto& token : filterResultsTokens_) {
    auto* inFilterResults(getProductSafe_(_inEvent, token));
    if (!inFilterResults)
      continue;

    auto&& filterNames(_inEvent.triggerNames(*inFilterResults));
    for (unsigned iF(0); iF != filterNames.size(); ++iF) {
      auto& name(filterNames.triggerName(iF));

      if (name == "Flag_HBHENoiseFilter")
        outMetFilters.hbhe = !inFilterResults->accept(iF);
      else if (name == "Flag_HBHENoiseIsoFilter")
        outMetFilters.hbheIso = !inFilterResults->accept(iF);
      else if (name == "Flag_EcalDeadCellTriggerPrimitiveFilter")
        outMetFilters.ecalDeadCell = !inFilterResults->accept(iF);
      else if (name == "Flag_eeBadScFilter")
        outMetFilters.badsc = !inFilterResults->accept(iF);
      else if (name == "Flag_globalTightHalo2016Filter")
        outMetFilters.globalHalo16 = !inFilterResults->accept(iF);
      else if (name == "Flag_goodVertices")
        outMetFilters.goodVertices = !inFilterResults->accept(iF);
      else if (name == "Flag_badMuons") // reverse convention
        outMetFilters.badMuons = inFilterResults->accept(iF);
      else if (name == "Flag_duplicateMuons") // reverse convention
        outMetFilters.duplicateMuons = inFilterResults->accept(iF);
      else if (name == "Flag_BadPFMuonFilter")
        outMetFilters.badPFMuons = !inFilterResults->accept(iF);
      else if (name == "Flag_BadChargedCandidateFilter")
        outMetFilters.badChargedHadrons = !inFilterResults->accept(iF);
      else if (name == "Flag_ecalBadCalibFilter")
        outMetFilters.ecalBadCalib = !inFilterResults->accept(iF);
    }
  }
}

DEFINE_TREEFILLER(MetFiltersFiller);
