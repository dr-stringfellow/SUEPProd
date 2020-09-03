import FWCore.ParameterSet.Config as cms

mergedGenParticles = cms.EDProducer("MergedGenProducer",
    inputPruned = cms.InputTag("prunedGenParticles"),
    inputPacked = cms.InputTag("packedGenParticles"),
)
