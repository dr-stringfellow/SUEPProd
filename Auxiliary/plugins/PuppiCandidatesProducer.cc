// -*- C++ -*-
// 
/**\class PuppiCandidatesProducer

   Description: Compute the maximum charged hadron isolation value over all PV hypotheses.

   Implementation:
   Create PF candidates with puppi weights from the input packed candidates.
*/
//
// Original Author:  Yutaro Iiyama
//         Created:  Mon, 26 Jun 2017 21:55:00 GMT
//
//

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/Common/interface/View.h"

#include "DataFormats/Candidate/interface/CandidateFwd.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"

#include "SUEPProd/Auxiliary/interface/getProduct.h"

#include <memory>
#include <vector>

class PuppiCandidatesProducer : public edm::stream::EDProducer<> {
public:
  explicit PuppiCandidatesProducer(const edm::ParameterSet&);
  ~PuppiCandidatesProducer();
  
private:
  void produce(edm::Event&, edm::EventSetup const&) override;

  typedef edm::ValueMap<reco::CandidatePtr> CandidatePtrMap;

  edm::EDGetTokenT<pat::PackedCandidateCollection> packedCandidatesToken_;
};

PuppiCandidatesProducer::PuppiCandidatesProducer(edm::ParameterSet const& _cfg) :
  packedCandidatesToken_(consumes<pat::PackedCandidateCollection>(_cfg.getParameter<edm::InputTag>("src")))
{
  produces<reco::PFCandidateCollection>();
  produces<reco::PFCandidateCollection>("noLep");
  produces<CandidatePtrMap>();
  produces<CandidatePtrMap>("noLep");
}

PuppiCandidatesProducer::~PuppiCandidatesProducer()
{
}

void
PuppiCandidatesProducer::produce(edm::Event& _event, edm::EventSetup const&)
{
  // Product
  std::unique_ptr<reco::PFCandidateCollection> output(new reco::PFCandidateCollection);
  std::unique_ptr<reco::PFCandidateCollection> outputNoLep(new reco::PFCandidateCollection);

  // Inputs
  edm::Handle<pat::PackedCandidateCollection> packedCandidatesHandle;
  auto& srcCandidates(*getProduct(_event, packedCandidatesToken_, &packedCandidatesHandle));

  static reco::PFCandidate const idTranslator;

  for (auto& cand : srcCandidates) {
    auto p4(cand.p4());
    output->emplace_back(cand.charge(), p4 * cand.puppiWeight(), idTranslator.translatePdgIdToType(cand.pdgId()));
    outputNoLep->emplace_back(cand.charge(), p4 * cand.puppiWeightNoLep(), idTranslator.translatePdgIdToType(cand.pdgId()));
  }

  auto orphanHandle(_event.put(std::move(output)));

  std::vector<reco::CandidatePtr> refToPuppi;
  refToPuppi.reserve(srcCandidates.size());
  for (unsigned iS(0); iS != srcCandidates.size(); ++iS)
    refToPuppi.emplace_back(orphanHandle, iS);

  std::unique_ptr<CandidatePtrMap> mapProduct(new CandidatePtrMap());
  CandidatePtrMap::Filler filler(*mapProduct);
  filler.insert(packedCandidatesHandle, refToPuppi.begin(), refToPuppi.end());
  filler.fill();
  _event.put(std::move(mapProduct));

  orphanHandle = _event.put(std::move(outputNoLep), "noLep");

  refToPuppi.clear();
  for (unsigned iS(0); iS != srcCandidates.size(); ++iS)
    refToPuppi.emplace_back(orphanHandle, iS);

  mapProduct.reset(new CandidatePtrMap());
  CandidatePtrMap::Filler fillerNoLep(*mapProduct);
  fillerNoLep.insert(packedCandidatesHandle, refToPuppi.begin(), refToPuppi.end());
  fillerNoLep.fill();
  _event.put(std::move(mapProduct), "noLep");
}

DEFINE_FWK_MODULE(PuppiCandidatesProducer);
