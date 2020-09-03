#ifndef SUEPProd_Producer_SecondaryVerticesFiller_h
#define SUEPProd_Producer_SecondaryVerticesFiller_h

#include "FillerBase.h"
#include "DataFormats/Candidate/interface/VertexCompositePtrCandidate.h"

class SecondaryVerticesFiller : public FillerBase {
 public:
  SecondaryVerticesFiller(std::string const&, edm::ParameterSet const&, edm::ConsumesCollector&);
  ~SecondaryVerticesFiller() {}

  void branchNames(suep::utils::BranchList&, suep::utils::BranchList&) const override;
  void fill(suep::Event&, edm::Event const&, edm::EventSetup const&) override;
  void setRefs(ObjectMapStore const&) override;

 protected:

  typedef edm::View<reco::VertexCompositePtrCandidate> SecondaryVertexView;
  NamedToken<SecondaryVertexView> secondaryVerticesToken_;

};

#endif
