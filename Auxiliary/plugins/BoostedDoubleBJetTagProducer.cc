// -*- C++ -*-
// 
/**\class BoostedDoubleBJetTagProducer

   Description: Double-b tagger.

   Implementation:
*/
//
// Original Author:  Yutaro Iiyama
//
//

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/Common/interface/View.h"
#include "DataFormats/Common/interface/RefToBaseProd.h"
#include "DataFormats/BTauReco/interface/JetTag.h"
#include "DataFormats/BTauReco/interface/BoostedDoubleSVTagInfo.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/Math/interface/deltaR.h"

#include "SUEPProd/Auxiliary/interface/getProduct.h"
#include "SUEPProd/Utilities/interface/BoostedBtaggingMVACalculator.h"

#include <memory>
#include <vector>

class BoostedDoubleBJetTagProducer : public edm::stream::EDProducer<> {
public:
  explicit BoostedDoubleBJetTagProducer(const edm::ParameterSet&);
  ~BoostedDoubleBJetTagProducer();
  
private:
  void produce(edm::Event&, edm::EventSetup const&) override;

  typedef edm::View<reco::Jet> JetView;

  edm::EDGetTokenT<JetView> jetsToken_;
  //  edm::EDGetTokenT<pat::JetCollection> subjetsToken_;
  edm::EDGetTokenT<JetView> subjetsToken_;
  edm::EDGetTokenT<reco::BoostedDoubleSVTagInfoCollection> btagInfoToken_;
  //  std::string subjetBtagTag_;
  edm::EDGetTokenT<reco::JetTagCollection> subjetBtagToken_;
  suep::BoostedBtaggingMVACalculator jetBoostedBtaggingMVACalc_;
  double subjetMatchRadius_;
};

BoostedDoubleBJetTagProducer::BoostedDoubleBJetTagProducer(edm::ParameterSet const& _cfg) :
  jetsToken_(consumes<JetView>(_cfg.getParameter<edm::InputTag>("jets"))),
  //  subjetsToken_(consumes<pat::JetCollection>(_cfg.getParameter<edm::InputTag>("subjets"))),
  subjetsToken_(consumes<JetView>(_cfg.getParameter<edm::InputTag>("subjets"))),
  btagInfoToken_(consumes<reco::BoostedDoubleSVTagInfoCollection>(_cfg.getParameter<edm::InputTag>("tagInfos"))),
  //  subjetBtagTag_(_cfg.getParameter<std::string>("subjetBtag")),
  subjetBtagToken_(consumes<reco::JetTagCollection>(_cfg.getParameter<edm::InputTag>("subjetBtag"))),
  jetBoostedBtaggingMVACalc_(),
  subjetMatchRadius_(_cfg.getParameter<double>("subjetMatchRadius"))
{
  jetBoostedBtaggingMVACalc_.initialize("BDT", _cfg.getParameter<edm::FileInPath>("weights").fullPath());
  produces<reco::JetTagCollection>();
}

BoostedDoubleBJetTagProducer::~BoostedDoubleBJetTagProducer()
{
}

void
BoostedDoubleBJetTagProducer::produce(edm::Event& _event, edm::EventSetup const&)
{
  auto* btagInfo(getProduct(_event, btagInfoToken_));

  edm::Handle<JetView> jetsHandle;
  getProduct(_event, jetsToken_, &jetsHandle);
  auto* subjets(getProduct(_event, subjetsToken_));
  auto* subjetBtag(getProduct(_event, subjetBtagToken_));

  auto out(std::make_unique<reco::JetTagCollection>(edm::RefToBaseProd<reco::Jet>(jetsHandle)));

  for (auto& dbi : *btagInfo) {
    auto&& ref(dbi.jet());
    auto& jet(*ref);
    auto&& vars(dbi.taggingVariables());

    double subjetCSVMin(999.);
    //    for (auto& subjet : *subjets) {
    for (unsigned iS(0); iS != subjets->size(); ++iS) {
      auto& subjet(subjets->at(iS));
      if (reco::deltaR(subjet.eta(), subjet.phi(), jet.eta(), jet.phi()) > subjetMatchRadius_) 
        continue;

      //double csv(subjet.bDiscriminator(subjetBtagTag_));
      double csv((*subjetBtag)[subjets->refAt(iS)]);
      if (csv < subjetCSVMin)
        subjetCSVMin = csv;
    }

    if (subjetCSVMin < -1. || subjetCSVMin > 1.)
      subjetCSVMin = -1.;

    (*out)[ref] = jetBoostedBtaggingMVACalc_.mvaValue(jet.mass(),
                                                      -1, //j.partonFlavor(); // they're spectator variables
                                                      -1, //j.hadronFlavor(); // 
                                                      jet.pt(),
                                                      jet.eta(),
                                                      subjetCSVMin,
                                                      vars.get(reco::btau::z_ratio),
                                                      vars.get(reco::btau::trackSip3dSig_3),
                                                      vars.get(reco::btau::trackSip3dSig_2),
                                                      vars.get(reco::btau::trackSip3dSig_1),
                                                      vars.get(reco::btau::trackSip3dSig_0),
                                                      vars.get(reco::btau::tau2_trackSip3dSig_0),
                                                      vars.get(reco::btau::tau1_trackSip3dSig_0),
                                                      vars.get(reco::btau::tau2_trackSip3dSig_1),
                                                      vars.get(reco::btau::tau1_trackSip3dSig_1),
                                                      vars.get(reco::btau::trackSip2dSigAboveCharm),
                                                      vars.get(reco::btau::trackSip2dSigAboveBottom_0),
                                                      vars.get(reco::btau::trackSip2dSigAboveBottom_1),
                                                      vars.get(reco::btau::tau1_trackEtaRel_0),
                                                      vars.get(reco::btau::tau1_trackEtaRel_1),
                                                      vars.get(reco::btau::tau1_trackEtaRel_2),
                                                      vars.get(reco::btau::tau2_trackEtaRel_0),
                                                      vars.get(reco::btau::tau2_trackEtaRel_1),
                                                      vars.get(reco::btau::tau2_trackEtaRel_2),
                                                      vars.get(reco::btau::tau1_vertexMass),
                                                      vars.get(reco::btau::tau1_vertexEnergyRatio),
                                                      vars.get(reco::btau::tau1_vertexDeltaR),
                                                      vars.get(reco::btau::tau1_flightDistance2dSig),
                                                      vars.get(reco::btau::tau2_vertexMass),
                                                      vars.get(reco::btau::tau2_vertexEnergyRatio),
                                                      vars.get(reco::btau::tau2_flightDistance2dSig),
                                                      vars.get(reco::btau::jetNTracks),
                                                      vars.get(reco::btau::jetNSecondaryVertices),
                                                      false);
   }
 
   _event.put(std::move(out));
}

DEFINE_FWK_MODULE(BoostedDoubleBJetTagProducer);
