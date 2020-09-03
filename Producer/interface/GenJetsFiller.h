#ifndef SUEPProd_Producer_GenJetsFiller_h
#define SUEPProd_Producer_GenJetsFiller_h

#include "FillerBase.h"

#include "DataFormats/JetReco/interface/GenJet.h"
#include "SimDataFormats/JetMatching/interface/JetFlavourInfoMatching.h"

class GenJetsFiller : public FillerBase {
 public:
  GenJetsFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~GenJetsFiller() {}

  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;
  void setRefs(ObjectMapStore const&) override;

 protected:
  typedef edm::View<reco::GenJet> GenJetView;
  typedef edm::ValueMap<int> IntMap;
  typedef edm::View<reco::GenParticle> GenParticleView;
  typedef edm::Ptr<reco::GenJet> GenJetPtr;

  NamedToken<GenParticleView> genParticlesToken_;
  NamedToken<GenJetView> genJetsToken_;
  NamedToken<reco::JetFlavourInfoMatchingCollection> flavorToken_;

  // GenHFHadronMatcher tokens
  NamedToken< std::vector<reco::GenParticle> >  genBHadPlusMothersToken_       ; // All mothers in all decay chains above any hadron of specified flavour
  NamedToken< std::vector< std::vector<int> > > genBHadPlusMothersIndicesToken_; // Indices of mothers of each hadMother
  NamedToken< std::vector<int> >                genBHadIndexToken_             ; // Index of hadron in the vector of hadMothers
  NamedToken< std::vector<int> >                genBHadFlavourToken_           ; // PdgId of the first non-b(c) quark mother with sign corresponding to hadron charge
  NamedToken< std::vector<int> >                genBHadJetIndexToken_          ; // Index of genJet matched to each hadron by jet clustering algorithm
  NamedToken< std::vector<int> >                genBHadLeptonIndexToken_       ; // Index of lepton found among the hadron decay products in the list of mothers
  NamedToken< std::vector<int> >                genBHadLeptonHadronIndexToken_ ; // Index of hadron the lepton is associated to
  NamedToken< std::vector<int> >                genBHadLeptonViaTauToken_      ; // Whether lepton comes directly from hadron or via tau decay
  NamedToken< std::vector<int> >                genBHadFromTopWeakDecayToken_  ; // Tells whether the hadron appears in the chain after top decay
  NamedToken< std::vector<int> >                genBHadBHadronIdToken_         ; // Index of a b-hadron which the current hadron comes from (for c-hadrons)
  
  NamedToken< std::vector<reco::GenParticle> >  genCHadPlusMothersToken_       ; // All mothers in all decay chains above any hadron of specified flavour
  NamedToken< std::vector< std::vector<int> > > genCHadPlusMothersIndicesToken_; // Indices of mothers of each hadMother
  NamedToken< std::vector<int> >                genCHadIndexToken_             ; // Index of hadron in the vector of hadMothers
  NamedToken< std::vector<int> >                genCHadFlavourToken_           ; // PdgId of the first non-b(c) quark mother with sign corresponding to hadron charge
  NamedToken< std::vector<int> >                genCHadJetIndexToken_          ; // Index of genJet matched to each hadron by jet clustering algorithm
  NamedToken< std::vector<int> >                genCHadLeptonIndexToken_       ; // Index of lepton found among the hadron decay products in the list of mothers
  NamedToken< std::vector<int> >                genCHadLeptonHadronIndexToken_ ; // Index of hadron the lepton is associated to
  NamedToken< std::vector<int> >                genCHadLeptonViaTauToken_      ; // Whether lepton comes directly from hadron or via tau decay
  NamedToken< std::vector<int> >                genCHadFromTopWeakDecayToken_  ; // Tells whether the hadron appears in the chain after top decay
  NamedToken< std::vector<int> >                genCHadBHadronIdToken_         ; // Index of a b-hadron which the current hadron comes from (for c-hadrons)

  typedef std::function<suep::GenJetCollection&(suep::Event&)> OutputSelector;

  OutputSelector outputSelector_{};

  double minPt_{15.};

  std::map<GenJetPtr, std::vector<reco::CandidatePtr>> jetBHadrons_;
  std::map<GenJetPtr, std::vector<reco::CandidatePtr>> jetCHadrons_;
};

#endif
