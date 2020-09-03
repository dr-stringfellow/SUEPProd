#include "../interface/PartonsFiller.h"

PartonsFiller::PartonsFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg)
{
  // Some samples have non-standard LHEEventProduct names
  // Using notifyNewProduct() to dynamically find the tag
  lheEventToken_.first = "lheEvent";
}

void
PartonsFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList&) const
{
  _eventBranches.push_back("partons");
}

void
PartonsFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const&)
{
  auto* lheEvent(getProductSafe_(_inEvent, lheEventToken_));
  if (!lheEvent)
    return;

  auto& outPartons(_outEvent.partons);

  auto& hepeup(lheEvent->hepeup());

  for (int iP(0); iP != hepeup.NUP; ++iP) {
    // only save status 1 partons
    if (hepeup.ISTUP[iP] != 1)
      continue;

    auto& outParton(outPartons.create_back());

    auto& pup(hepeup.PUP[iP]);
    outParton.setXYZE(pup[0], pup[1], pup[2], pup[3]);

    outParton.pdgid = hepeup.IDUP[iP];
  }
}

void
PartonsFiller::notifyNewProduct(edm::BranchDescription const& _bdesc, edm::ConsumesCollector& _coll)
{
  if (_bdesc.unwrappedTypeID() == edm::TypeID(typeid(LHEEventProduct))) {
    edm::InputTag tag(_bdesc.moduleLabel(), _bdesc.productInstanceName(), _bdesc.processName());
    lheEventToken_.second = _coll.consumes<LHEEventProduct>(tag);
  }
}

DEFINE_TREEFILLER(PartonsFiller);
