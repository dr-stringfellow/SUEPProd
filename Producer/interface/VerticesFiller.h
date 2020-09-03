#ifndef SUEPProd_Producer_VerticesFiller_h
#define SUEPProd_Producer_VerticesFiller_h

#include "FillerBase.h"

#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/Candidate/interface/CandidateFwd.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"

#include "TH1D.h"

class VerticesFiller : public FillerBase {
 public:
  VerticesFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~VerticesFiller() {}

  void branchNames(suep::utils::BranchList&, suep::utils::BranchList&) const override;
  void addOutput(TFile&) override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;
  void fillAll(edm::Event const&, edm::EventSetup const&) override;

 protected:
  typedef edm::View<reco::Vertex> VertexView;
  typedef edm::ValueMap<float> VertexScore;
  typedef std::vector<PileupSummaryInfo> PUSummaryCollection;
  typedef edm::View<reco::GenParticle> GenParticleView;

  NamedToken<VertexView> verticesToken_;
  NamedToken<VertexScore> scoresToken_;
  NamedToken<reco::CandidateView> candidatesToken_;
  NamedToken<PUSummaryCollection> puSummariesToken_;
  NamedToken<GenParticleView> genParticlesToken_; // for genVertex

  TH1D* hNPVReco_{0};
  TH1D* hNPVTrue_{0};

  //! fillAll and fill will collect identical information -> cache it in fillAll
  unsigned short npvCache_{0};
  unsigned short npvTrueCache_{0};
};

#endif
