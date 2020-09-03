#ifndef SUEPProd_Producer_FatJetsFiller_h
#define SUEPProd_Producer_FatJetsFiller_h

#include "JetsFiller.h"

#include "DataFormats/BTauReco/interface/JetTag.h"
#include "SUEPProd/Utilities/interface/HEPTopTaggerWrapperV2.h"
#include "SUEPProd/Utilities/interface/EnergyCorrelations.h"

// fastjet
#include "fastjet/PseudoJet.hh"
#include "fastjet/JetDefinition.hh"
#include "fastjet/GhostedAreaSpec.hh"
#include "fastjet/AreaDefinition.hh"
#include "fastjet/ClusterSequenceArea.hh"
#include "fastjet/contrib/SoftDrop.hh"
#include "fastjet/contrib/NjettinessPlugin.hh"
#include "fastjet/contrib/MeasureDefinition.hh"
#include "fastjet/contrib/EnergyCorrelator.hh"

class FatJetsFiller : public JetsFiller {
 public:
  FatJetsFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~FatJetsFiller();

  void branchNames(suep::utils::BranchList& eventBranches, suep::utils::BranchList&) const override;

 protected:
  void fillDetails_(suep::Event&, edm::Event const&, edm::EventSetup const&) override;

  NamedToken<JetView> subjetsToken_;
  NamedToken<int> categoriesToken_;
  std::string shallowBBTagTag_;
  std::string deepBBprobQTag_;
  std::string deepBBprobHTag_;
  std::string njettinessTag_;
  std::string sdKinematicsTag_;
  std::string prunedKinematicsTag_;
  std::string subjetBtagTag_;
  std::string subjetQGLTag_;

  std::string subjetCmvaTag_;
  std::string subjetDeepCsvTag_;
  std::string subjetDeepCmvaTag_;

  fastjet::GhostedAreaSpec activeArea_;
  fastjet::AreaDefinition areaDef_;
  fastjet::JetDefinition* jetDefCA_{0};
  fastjet::contrib::SoftDrop* softdrop_{0};
  fastjet::contrib::Njettiness* tau_{0};
  fastjet::HEPTopTaggerV2* htt_{0};
  suepecf::Calculator *ecfcalc_{0};

  enum SubstructureComputeMode {
    kAlways,
    kLargeRecoil,
    kNever
  };

  typedef std::function<suep::MicroJetCollection&(suep::Event&)> OutSubjetSelector;

  OutSubjetSelector outSubjetSelector_{};

  SubstructureComputeMode computeSubstructure_{kNever};
};

#endif
