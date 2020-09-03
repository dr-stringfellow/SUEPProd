#include "../interface/RecoilFiller.h"

RecoilFiller::RecoilFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg)
{
  getToken_(categoriesToken_, _cfg, _coll, "categories");
  getToken_(maxToken_, _cfg, _coll, "max");
}

void
RecoilFiller::addOutput(TFile& _outputFile)
{
  TDirectory::TContext context(&_outputFile);
  auto* t(suep::utils::makeDocTree("RecoilCategory", suep::Recoil::CategoryName, suep::Recoil::nCategories));
  t->Write();
  delete t;
}

void
RecoilFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList&) const
{
  _eventBranches.emplace_back("recoil");
}

void
RecoilFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const&)
{
  int categories(getProduct_(_inEvent, categoriesToken_));
  auto& recoil(_outEvent.recoil);

  recoil.met = ((categories >> suep::Recoil::rMET) & 1) != 0;
  recoil.monoMu = ((categories >> suep::Recoil::rMonoMu) & 1) != 0;
  recoil.monoE = ((categories >> suep::Recoil::rMonoE) & 1) != 0;
  recoil.diMu = ((categories >> suep::Recoil::rDiMu) & 1) != 0;
  recoil.diE = ((categories >> suep::Recoil::rDiE) & 1) != 0;
  recoil.gamma = ((categories >> suep::Recoil::rGamma) & 1) != 0;

  recoil.max = getProduct_(_inEvent, maxToken_);
}

DEFINE_TREEFILLER(RecoilFiller);
