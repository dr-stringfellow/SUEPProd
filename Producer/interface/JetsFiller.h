#ifndef SUEPProd_Producer_JetsFiller_h
#define SUEPProd_Producer_JetsFiller_h

#include "FillerBase.h"

#include "DataFormats/Common/interface/View.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/JetReco/interface/Jet.h"
#include "DataFormats/JetReco/interface/GenJetCollection.h"
#include "DataFormats/PatCandidates/interface/Jet.h"

#include <functional>

class JetCorrectionUncertainty;

class JetsFiller : public FillerBase {
 public:
  JetsFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~JetsFiller();

  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;
  void setRefs(ObjectMapStore const&) override;

 protected:
  virtual void fillDetails_(suep::Event&, edm::Event const&, edm::EventSetup const&) {}
  void fillDeepBySwitch_(suep::MicroJet&, unsigned int const, float);

  typedef edm::View<reco::Jet> JetView;
  typedef edm::View<reco::GenJet> GenJetView;

  NamedToken<JetView> jetsToken_;
  NamedToken<edm::View<pat::Jet>> puidJetsToken_;
  NamedToken<GenJetView> genJetsToken_;
  NamedToken<double> rhoToken_;
  std::string jecName_;
  std::string jerName_;
  std::string csvTag_;
  std::string cmvaTag_;
  std::string qglTag_;

  enum deepSuff {
    udsg = 0,
    b,
    c,
    bb,
    cc,
    DEEP_SIZE
  };

  // Add suffixes to maps
#define ADD_TO_MAP(s) {#s, deepSuff::s}
  const std::map<const std::string, deepSuff> deepProbs = {
    ADD_TO_MAP(udsg),
    ADD_TO_MAP(b),
    ADD_TO_MAP(c),
    ADD_TO_MAP(bb),
    ADD_TO_MAP(cc)
  };

  std::string deepCsvTag_;
  std::string deepCmvaTag_;

  std::string puidTag_;

  JetCorrectionUncertainty* jecUncertainty_{0};

  typedef std::function<suep::JetCollection&(suep::Event&)> OutputSelector;

  OutputSelector outputSelector_{};

  std::string outGenJets_{};
  std::string constituentsLabel_{};
  //! jet radius
  double R_{0.4};
  double minPt_{15.};
  double maxEta_{4.7};

  bool fillConstituents_{false};
  unsigned subjetsOffset_{0}; // first N constituents are actually subjets (happens when fixDaughters = True in JetSubstructurePacker)
};

#endif
