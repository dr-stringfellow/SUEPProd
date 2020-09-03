#ifndef SUEPProd_Producer_ElectronsFiller_h
#define SUEPProd_Producer_ElectronsFiller_h

#include "FillerBase.h"

#include "DataFormats/Common/interface/View.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/EgammaCandidates/interface/PhotonFwd.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectronFwd.h"
#include "DataFormats/EgammaCandidates/interface/ConversionFwd.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
#include "DataFormats/EgammaReco/interface/SuperClusterFwd.h"
#include "DataFormats/Candidate/interface/CandidateFwd.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"

#include "RecoEgamma/EgammaTools/interface/EffectiveAreas.h"

#include <vector>
#include <utility>

class ElectronsFiller : public FillerBase {
 public:
  ElectronsFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~ElectronsFiller() {}

  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;
  void setRefs(ObjectMapStore const&) override;

 protected:
  typedef edm::View<reco::Vertex> VertexView;
  typedef edm::View<reco::Photon> PhotonView;
  typedef edm::View<reco::GsfElectron> GsfElectronView;
  typedef edm::ValueMap<bool> BoolMap;
  typedef edm::ValueMap<int> IntMap;
  typedef edm::ValueMap<float> FloatMap;

  NamedToken<GsfElectronView> electronsToken_;
  NamedToken<GsfElectronView> smearedElectronsToken_;
  NamedToken<GsfElectronView> regressionElectronsToken_;
  NamedToken<PhotonView> photonsToken_;
  NamedToken<reco::ConversionCollection> conversionsToken_;
  NamedToken<reco::CandidateView> pfCandidatesToken_;
  NamedToken<EcalRecHitCollection> ebHitsToken_;
  NamedToken<EcalRecHitCollection> eeHitsToken_;
  NamedToken<reco::BeamSpot> beamSpotToken_;
  NamedToken<BoolMap> vetoIdToken_;
  NamedToken<BoolMap> looseIdToken_;
  NamedToken<BoolMap> mediumIdToken_;
  NamedToken<BoolMap> tightIdToken_;
  NamedToken<BoolMap> hltIdToken_;
  NamedToken<BoolMap> mvaWP90Token_;
  NamedToken<BoolMap> mvaWP80Token_;
  NamedToken<BoolMap> mvaWPLooseToken_;
  NamedToken<BoolMap> mvaIsoWP90Token_;
  NamedToken<BoolMap> mvaIsoWP80Token_;
  NamedToken<BoolMap> mvaIsoWPLooseToken_;
  NamedToken<FloatMap> mvaValuesMapToken_;
  //NamedToken<IntMap> mvaCategoriesMapToken_;

  NamedToken<FloatMap> phCHIsoToken_;
  NamedToken<FloatMap> phNHIsoToken_;
  NamedToken<FloatMap> phPhIsoToken_;
  NamedToken<VertexView> verticesToken_;
  // Iso tokens only used if filling from AOD
  NamedToken<FloatMap> ecalIsoToken_;
  NamedToken<FloatMap> hcalIsoToken_;
  NamedToken<double> rhoToken_;
  NamedToken<double> rhoCentralCaloToken_;

  EffectiveAreas combIsoEA_;
  EffectiveAreas ecalIsoEA_;
  EffectiveAreas hcalIsoEA_;
  EffectiveAreas phCHIsoEA_;
  EffectiveAreas phNHIsoEA_;
  EffectiveAreas phPhIsoEA_;
};

#endif
