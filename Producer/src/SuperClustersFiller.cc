#include "../interface/SuperClustersFiller.h"

#include "RecoEcal/EgammaCoreTools/interface/EcalClusterLazyTools.h"
#include "DataFormats/EcalDetId/interface/EcalSubdetector.h"
#include "DataFormats/EgammaReco/interface/SuperCluster.h"

#include <cmath>

SuperClustersFiller::SuperClustersFiller(std::string const& _name, edm::ParameterSet const& _cfg, edm::ConsumesCollector& _coll) :
  FillerBase(_name, _cfg)
{
  getToken_(superClustersToken_, _cfg, _coll, "superClusters");
  getToken_(ebHitsToken_, _cfg, _coll, "ebHits");
  getToken_(eeHitsToken_, _cfg, _coll, "eeHits");
}

void
SuperClustersFiller::branchNames(suep::utils::BranchList& _eventBranches, suep::utils::BranchList&) const
{
  _eventBranches.emplace_back("superClusters");
}

void
SuperClustersFiller::fill(suep::Event& _outEvent, edm::Event const& _inEvent, edm::EventSetup const& _setup)
{
  auto& inSuperClusters(getProduct_(_inEvent, superClustersToken_));

  noZS::EcalClusterLazyTools lazyTools(_inEvent, _setup, ebHitsToken_.second, eeHitsToken_.second);

  auto& outSuperClusters(_outEvent.superClusters);
  outSuperClusters.reserve(inSuperClusters.size());

  auto& objectMap(objectMap_->get<reco::SuperCluster, suep::SuperCluster>());

  unsigned iSC(-1);
  for (auto& inSC : inSuperClusters) {
    ++iSC;
    auto& outSC(outSuperClusters.create_back());

    outSC.eta = inSC.eta();
    outSC.phi = inSC.phi();
    outSC.rawPt = inSC.rawEnergy() / std::cosh(outSC.eta);

    objectMap.add(inSuperClusters.ptrAt(iSC), outSC);
  }
}

DEFINE_TREEFILLER(SuperClustersFiller);
