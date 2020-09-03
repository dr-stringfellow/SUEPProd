#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Common/interface/View.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "SUEPProd/Auxiliary/interface/PackedValuesExposer.h"
#include <vector>

#include "HepPDT/ParticleID.hh"

class MergedGenProducer : public edm::stream::EDProducer<>
{
 public:
   MergedGenProducer(const edm::ParameterSet& pset);
   ~MergedGenProducer() {};

 private:
   void produce(edm::Event& event, const edm::EventSetup&) override;
   bool isPhotonFromPrunedHadron(const pat::PackedGenParticle& pk) const;
   
   edm::EDGetTokenT<edm::View<reco::GenParticle>> input_pruned_;
   edm::EDGetTokenT<edm::View<pat::PackedGenParticle>> input_packed_;
};


MergedGenProducer::MergedGenProducer(const edm::ParameterSet& config)
{
  input_pruned_ = consumes<edm::View<reco::GenParticle>>(config.getParameter<edm::InputTag>("inputPruned"));
  input_packed_ = consumes<edm::View<pat::PackedGenParticle>>(config.getParameter<edm::InputTag>("inputPacked"));

  produces<reco::GenParticleCollection>();
}

void MergedGenProducer::produce(edm::Event& event, const edm::EventSetup& setup)
{
  // Need a ref to the product now for creating the mother/daughter refs
  auto ref = event.getRefBeforePut<reco::GenParticleCollection>();

  // Get the input collections
  edm::Handle<edm::View<reco::GenParticle> > pruned_handle;
  event.getByToken(input_pruned_, pruned_handle);

  edm::Handle<edm::View<pat::PackedGenParticle> > packed_handle;
  event.getByToken(input_packed_, packed_handle);

  // First determine which packed particles are also still in the pruned collection
  // so that we can skip them later
  // std::map<reco::GenParticle const*, pat::PackedGenParticle const*> st1_dup_map;
  std::map<pat::PackedGenParticle const*, reco::GenParticle const*> st1_dup_map;

  // Also map pointers in the original pruned collection to their index in the vector.
  // This index will be the same in the merged collection.
  std::map<reco::Candidate const*, std::size_t> pruned_idx_map;
  for (unsigned int i = 0; i < pruned_handle->size(); ++i) {
    reco::GenParticle const& pr = pruned_handle->at(i);
    pruned_idx_map[&pr] = i;
    if (pr.status() != 1) continue;

    unsigned found_matches = 0;
    for (unsigned j = 0; j < packed_handle->size(); ++j) {
      // Compare explicitly the integral values of the packed dudes
      pat::PackedGenParticle const& pk = packed_handle->at(j);
      pat::PackedGenParticle packedPruned(pr);
      PackedGenParticleExposer exposedPacked(pk), exposedPackedPruned(packedPruned);
      if (
        pr.pdgId() != pk.pdgId()  
        || exposedPacked.packedPt()!=exposedPackedPruned.packedPt()
        || exposedPacked.packedY()!=exposedPackedPruned.packedY()
        || exposedPacked.packedPhi()!=exposedPackedPruned.packedPhi()
        || exposedPacked.packedM()!=exposedPackedPruned.packedM()
      ) continue;
      ++found_matches;
      st1_dup_map[&pk] = &pr;
    }
    if (found_matches > 1) {
      edm::LogWarning("MergedGenProducer") << "Found multiple packed matches for: " << i << "\t" << pr.pdgId() << "\t" << pr.pt() << "\t" << pr.y() << "\n";
    }
    else if (found_matches == 0 && std::abs(pr.y()) < 6.0) {
      edm::LogWarning("MergedGenProducer") << "unmatched status 1: " << i << "\t" << pr.pdgId() << "\t" << pr.pt() << "\t" << pr.y() << "\n";
    }
  }

  // Fix by Markus
  // check for photons from pruned (light) hadrons
  unsigned int nPhotonsFromPrunedHadron = 0;
  for (unsigned int j = 0; j < packed_handle->size(); ++j) {
    pat::PackedGenParticle const& pk = packed_handle->at(j);
    if (isPhotonFromPrunedHadron(pk)) ++nPhotonsFromPrunedHadron;
  }

  // At this point we know what the size of the merged GenParticle will be so we can create it
  const unsigned int n = pruned_handle->size() + (packed_handle->size() - st1_dup_map.size()) + nPhotonsFromPrunedHadron;
  auto cands = std::unique_ptr<reco::GenParticleCollection>(new reco::GenParticleCollection(n));

  // First copy in all the pruned candidates
  for (unsigned i = 0; i < pruned_handle->size(); ++i) {
    reco::GenParticle const& old_cand = pruned_handle->at(i);
    reco::GenParticle & new_cand = cands->at(i);
    new_cand = reco::GenParticle(pruned_handle->at(i));
    // Update the mother and daughter refs to this new merged collection
    new_cand.resetMothers(ref.id());
    new_cand.resetDaughters(ref.id());
    for (unsigned m = 0; m < old_cand.numberOfMothers(); ++m) {
      new_cand.addMother(reco::GenParticleRef(ref, pruned_idx_map.at(old_cand.mother(m))));
    }
    for (unsigned d = 0; d < old_cand.numberOfDaughters(); ++d) {
      new_cand.addDaughter(reco::GenParticleRef(ref, pruned_idx_map.at(old_cand.daughter(d))));
    }
  }

  // Now copy in the packed candidates that are not already in the pruned
  for (unsigned i = 0, idx = pruned_handle->size(); i < packed_handle->size(); ++i) {
    pat::PackedGenParticle const& pk = packed_handle->at(i);
    if (st1_dup_map.count(&pk)) continue;
    reco::GenParticle & new_cand = cands->at(idx);
    new_cand = reco::GenParticle(pk.charge(), pk.p4(), pk.vertex(), pk.pdgId(), 1, true);

    // Insert dummy pi0 mothers for orphaned photons
    if (isPhotonFromPrunedHadron(pk)) {
      ++idx;
      reco::GenParticle & dummy_mother = cands->at(idx);
      dummy_mother = reco::GenParticle(0, pk.p4(), pk.vertex(), 111, 2, true);
      for (unsigned m = 0; m < pk.numberOfMothers(); ++m) {
        new_cand.addMother(reco::GenParticleRef(ref, idx));
        // Since the packed candidates drop the vertex position we'll take this from the mother
        if (m == 0) {
          dummy_mother.setP4(pk.mother(m)->p4());
          dummy_mother.setVertex(pk.mother(m)->vertex());
          new_cand.setVertex(pk.mother(m)->vertex());
        }
        // Should then add the GenParticle as a daughter of its dummy mother
        dummy_mother.addDaughter(reco::GenParticleRef(ref, idx-1));
      }
    }
    // Connect to mother from pruned particles
    reco::GenParticle & daughter = cands->at(idx);
    for (unsigned m = 0; m < pk.numberOfMothers(); ++m) {
      daughter.addMother(reco::GenParticleRef(ref, pruned_idx_map.at(pk.mother(m))));
      // Since the packed candidates drop the vertex position we'll take this from the mother
      if (m == 0) {
        daughter.setVertex(pk.mother(m)->vertex());
      }
      // Should then add this GenParticle as a daughter of its mother
      cands->at(pruned_idx_map.at(pk.mother(m))).addDaughter(reco::GenParticleRef(ref, idx));
    }
    ++idx;
  }

  event.put(std::move(cands));
}

bool MergedGenProducer::isPhotonFromPrunedHadron(const pat::PackedGenParticle& pk) const
{
  if (pk.pdgId() == 22 and pk.statusFlags().isDirectHadronDecayProduct()) {
    // no mother
    if (pk.numberOfMothers() == 0) return true;
    // miniaod mother not compatible with the status flag
    HepPDT::ParticleID motherid(pk.mother(0)->pdgId());
    if (not (motherid.isHadron() and pk.mother(0)->status() == 2)) return true;
  }
  return false;
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(MergedGenProducer);
