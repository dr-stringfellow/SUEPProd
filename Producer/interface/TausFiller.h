#ifndef SUEPProd_Producer_TausFiller_h
#define SUEPProd_Producer_TausFiller_h

#include "FillerBase.h"

#include "DataFormats/Common/interface/View.h"
#include "DataFormats/TauReco/interface/BaseTau.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"

class TausFiller : public FillerBase {
 public:
  TausFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~TausFiller() {}

  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;
  void setRefs(ObjectMapStore const&) override;

 protected:
  typedef edm::View<reco::BaseTau> TauView;
  typedef edm::View<reco::GenParticle> GenParticleView;

  NamedToken<TauView> tausToken_;
  NamedToken<GenParticleView> genParticlesToken_;
};

#endif
