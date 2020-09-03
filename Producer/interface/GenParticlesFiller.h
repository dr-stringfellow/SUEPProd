#ifndef SUEPProd_Producer_GenParticlesFiller_h
#define SUEPProd_Producer_GenParticlesFiller_h

#include "FillerBase.h"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"

#include "SUEPTree/Objects/interface/UnpackedGenParticle.h"

class GenParticlesFiller : public FillerBase {
 public:
  GenParticlesFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~GenParticlesFiller() {}

  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void addOutput(TFile&) override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;

 protected:
  typedef edm::View<reco::GenParticle> GenParticleView;
  typedef edm::View<pat::PackedGenParticle> PackedGenParticleView;

  NamedToken<GenParticleView> genParticlesToken_;
  NamedToken<PackedGenParticleView> finalStateParticlesToken_;

  bool furtherPrune_{true};
  bool fillPacked_{true};
  bool fillUnpacked_{false};

  suep::UnpackedGenParticleCollection outUnpacked = suep::UnpackedGenParticleCollection("genParticlesU", 256);
  TTree* outputTree_{0};
};

#endif
