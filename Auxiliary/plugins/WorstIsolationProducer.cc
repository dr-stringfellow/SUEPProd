// -*- C++ -*-
// 
/**\class WorstIsolationProducer

   Description: Compute the maximum charged hadron isolation value over all PV hypotheses.

   Implementation:
   Main body is copied from
     RecoEgamma/PhotonIdentification/plugins/PhotonIDValueMapProducer.cc
   The original code hard-codes the vertex index (0); this one loops over it.
*/
//
// Original Author:  Yutaro Iiyama
//         Created:  Thu, 29 Dec 2016 21:25:44 GMT
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
#include "DataFormats/EgammaCandidates/interface/Photon.h"
#include "DataFormats/PatCandidates/interface/Photon.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"

#include "SUEPProd/Auxiliary/interface/getProduct.h"

#include <memory>
#include <vector>

typedef edm::View<reco::Photon> PhotonView;
typedef edm::View<reco::Candidate> CandidateView;
typedef std::vector<reco::PFCandidateRef> Footprint;
typedef edm::ValueMap<Footprint> FootprintMap;
typedef edm::ValueMap<float> FloatMap;

// Templated isolation calculator (for different types of footprint collections)
template<class F>
bool
isInFootprint(reco::CandidatePtr const& _ptr, F const& _footprint)
{
  for (auto& fp : _footprint) {
    if(fp.key() == _ptr.key())
      return true;
  }
  return false;
}

void
getImpactParameters(reco::Candidate const& _ptr, reco::Vertex const& _vtx, bool _isPAT, double& _dxy, double& _dz)
{
  if (_isPAT) {
    auto& cand(static_cast<pat::PackedCandidate const&>(_ptr));
    _dxy = cand.dxy(_vtx.position());
    _dz = cand.dz(_vtx.position());
  }
  else {
    auto& track(*static_cast<reco::PFCandidate const&>(_ptr).trackRef());
    _dxy = track.dxy(_vtx.position());
    _dz  = track.dz(_vtx.position());
  }
}

class WorstIsolationProducer : public edm::stream::EDProducer<> {
public:
  explicit WorstIsolationProducer(const edm::ParameterSet&);
  ~WorstIsolationProducer();
  
private:
  void produce(edm::Event&, edm::EventSetup const&) override;

  edm::EDGetTokenT<PhotonView> photonsToken_;
  edm::EDGetTokenT<CandidateView> pfCandidatesToken_;
  edm::EDGetTokenT<reco::VertexCollection> vtxToken_;
  edm::EDGetTokenT<FootprintMap> footprintMapToken_;
};

WorstIsolationProducer::WorstIsolationProducer(edm::ParameterSet const& _cfg) :
  photonsToken_(consumes<PhotonView>(_cfg.getParameter<edm::InputTag>("photons"))),
  pfCandidatesToken_(consumes<CandidateView>(_cfg.getParameter<edm::InputTag>("pfCandidates"))),
  vtxToken_(consumes<reco::VertexCollection>(_cfg.getParameter<edm::InputTag>("vertices")))
{
  if (_cfg.exists("footprintMap"))
    footprintMapToken_ = mayConsume<FootprintMap>(_cfg.getParameter<edm::InputTag>("footprintMap"));

  produces<FloatMap>();
}

WorstIsolationProducer::~WorstIsolationProducer()
{
}

void
WorstIsolationProducer::produce(edm::Event& _event, edm::EventSetup const&)
{
  // Constants 
  double const coneSizeDR2 = 0.3 * 0.3;
  double const dxyMax = 0.1;
  double const dzMax  = 0.2;

  // Product
  std::vector<float> worstIsolations;

  edm::Handle<PhotonView> photonsHandle;

  // Write function
  auto writeProduct([&_event, &photonsHandle, &worstIsolations] {
      auto valueMap = std::make_unique<FloatMap>();
      FloatMap::Filler filler(*valueMap);
      filler.insert(photonsHandle, worstIsolations.begin(), worstIsolations.end());
      filler.fill();
      _event.put(std::move(valueMap));
    });

  // Inputs
  // photons
  auto& photons(*getProduct(_event, photonsToken_, &photonsHandle));

  if (photons.size() == 0) {
    writeProduct();
    return;
  }

  // not the most elegant test but it works..
  bool isPAT(dynamic_cast<pat::Photon const*>(&photons.at(0)) != 0);

  // candidates
  auto& pfCandidates(*getProduct(_event, pfCandidatesToken_));
  if (pfCandidates.size() == 0) {
    worstIsolations.assign(photons.size(), 0.);
    writeProduct();
    return;
  }

  if (isPAT && dynamic_cast<pat::PackedCandidate const*>(&pfCandidates.at(0)) == 0)
    throw cms::Exception("InconsistentInput") << "PAT photon with non-packed candidates";

  // vertices
  auto& vertices(*getProduct(_event, vtxToken_));

  // footprints
  FootprintMap const* footprintMap(0);
  if(!isPAT){
    // PAT photons already contain footprint information
    footprintMap = getProduct(_event, footprintMapToken_);
  }

  // First, group the charged hadrons by vertex
  std::vector<std::vector<unsigned>> chIndicesByVertex(vertices.size());
  for (unsigned iPF(0); iPF != pfCandidates.size(); ++iPF) {
    auto& cand(pfCandidates.at(iPF));

    if (isPAT) {
      if (std::abs(cand.pdgId()) != 211)
        continue;
    }
    else {
      if (static_cast<reco::PFCandidate const&>(cand).particleId() != reco::PFCandidate::h)
        continue;
    }

    double dxy(999.);
    double dz(999.);

    for (unsigned iV(0); iV != vertices.size(); ++iV) {
      auto& vtx(vertices.at(iV));

      getImpactParameters(cand, vtx, isPAT, dxy, dz);

      if (dxy > dxyMax)
        continue;
      if (dz > dzMax)
        continue;

      chIndicesByVertex[iV].push_back(iPF);

      // not breaking - allow one track to be associated with multiple vertices
    }
  }

  // Loop over photons
  for (unsigned iPh(0); iPh != photons.size(); ++iPh) {
    auto& photon(photons.at(iPh));
    auto& sc(*photon.superCluster());

    Footprint const* recoFootprint(0);
    edm::RefVector<pat::PackedCandidateCollection> patFootprint;
    if (isPAT)
      patFootprint = static_cast<pat::Photon const&>(photon).associatedPackedPFCandidates();
    else
      recoFootprint = &(*footprintMap)[photons.ptrAt(iPh)];

    double worstIso(0.);

    // Loop over all vertices
    for (unsigned iV(0); iV != vertices.size(); ++iV) {
      auto& vtx(vertices.at(iV));

      // Compute photon direction with respect to the vertex
      math::XYZVector direction(sc.x() - vtx.x(), sc.y() - vtx.y(), sc.z() - vtx.z());

      // Add pT of the charged hadrons in dR cone and not in the footprint
      double isoSum(0.);

      for (unsigned iPF : chIndicesByVertex[iV]) {
        auto& cand(pfCandidates.at(iPF));

        // Check if this candidate is within the isolation cone
        double dR2(deltaR2(direction.Eta(), direction.Phi(), cand.eta(), cand.phi()));
        if (dR2 > coneSizeDR2)
          continue;

        auto candPtr(pfCandidates.ptrAt(iPF));
        if (isPAT) {
          if (isInFootprint(candPtr, patFootprint))
            continue;
        }
        else {
          if (isInFootprint(candPtr, *recoFootprint))
            continue;
        }

        isoSum += cand.pt();
      }

      if (isoSum > worstIso)
        worstIso = isoSum;
    }

    // Push back the worst iso value
    worstIsolations.push_back(worstIso);
  }

  writeProduct();
}

DEFINE_FWK_MODULE(WorstIsolationProducer);
