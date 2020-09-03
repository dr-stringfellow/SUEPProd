#ifndef SUEPProd_Auxiliary_PackedValuesExposer_h
#define SUEPProd_Auxiliary_PackedValuesExposer_h

#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"

//! Expose packed values which have no public accessor but are protected members
class PackedPatCandidateExposer : public pat::PackedCandidate {
 public:
  PackedPatCandidateExposer(pat::PackedCandidate const& cand) : pat::PackedCandidate(cand) {}
  uint16_t packedPt() const { return packedPt_; }
  uint16_t packedEta() const { return packedEta_; }
  uint16_t packedPhi() const { return packedPhi_; }
  uint16_t packedM() const { return packedM_; }
  int16_t packedPuppiweight() const { return packedPuppiweight_; }
  int16_t packedPuppiweightNoLepDiff() const { return packedPuppiweightNoLepDiff_; }
  uint16_t packedDxy() const { return packedDxy_; }
  uint16_t packedDz() const { return packedDz_; }
  uint16_t packedDPhi() const { return packedDPhi_; }
};

//! Expose packed values which have no public accessor but are protected members
class PackedGenParticleExposer : public pat::PackedGenParticle {
 public:
  PackedGenParticleExposer(pat::PackedGenParticle const& part) : pat::PackedGenParticle(part) {}
  uint16_t packedPt() const { return packedPt_; }
  uint16_t packedY() const { return packedY_; }
  uint16_t packedPhi() const { return packedPhi_; }
  uint16_t packedM() const { return packedM_; }
};

#endif
