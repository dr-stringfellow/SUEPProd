/*!
  WeightsFiller
  Fill event weights. Assumes weights with id 1-9 (LO) or 1001-1009 (NLO) are mu_R and mu_F variations mu_R = (1, 2, 0.5) x mu_F = (1, 2, 0.5)
  and those with id 11-110 (LO) or 2001-2100 (NLO) are NNPDF MC variations (only RMS interesting).
*/

#ifndef SUEPProd_Producer_WeightsFiller_h
#define SUEPProd_Producer_WeightsFiller_h

#include "FillerBase.h"

#include "FWCore/Framework/interface/GetterOfProducts.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/LHEEventProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/LHERunInfoProduct.h"

#include "TH1D.h"
#include "TObjString.h"

class WeightsFiller : public FillerBase {
 public:
  WeightsFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~WeightsFiller() {}

  void branchNames(suep::utils::BranchList&, suep::utils::BranchList&) const override;
  void addOutput(TFile&) override;
  void fillAll(edm::Event const&, edm::EventSetup const&) override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;
  void fillEndRun(suep::Run&, edm::Run const&, edm::EventSetup const&) override;
  void notifyNewProduct(edm::BranchDescription const&, edm::ConsumesCollector&) override;

 protected:
  void getLHEWeights_(LHEEventProduct const&);
  void bookGenParam_();

  NamedToken<GenEventInfoProduct> genInfoToken_;
  NamedToken<LHEEventProduct> lheEventToken_;

  // learn the size of the signal weights vector in the first 100 events
  static unsigned const learningPhase{100};
  
  std::vector<TString> wids_{};
  float genParamBuffer_[learningPhase][suep::GenReweight::NMAX]{};
  unsigned bufferCounter_{0};

  unsigned pdfBegin_{0};
  unsigned pdfEnd_{0};

  double central_{0.};
  double normScaleVariations_[6]{}; // Scale variations (muR, muF) normalized by originalXWGTUP
  double normPDFVariations_[100]{}; // NNPDF variations normalized by originalXWGTUP
  float genParam_[suep::GenReweight::NMAX]{}; // I don't like that we hard-code the array size here..

  // these objects will be deleted automatically when the output file closes
  TH1D* hSumW_{0};

  // need to hold on to the output file handle
  TFile* outputFile_{0};
};

#endif
