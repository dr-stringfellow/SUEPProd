import FWCore.ParameterSet.Config as cms

worstIsolationProducer = cms.EDProducer('WorstIsolationProducer',
    photons = cms.InputTag('slimmedPhotons'),
    pfCandidates = cms.InputTag('packedPFCandidates'),
    vertices = cms.InputTag('offlineSlimmedPrimaryVertices')
)
