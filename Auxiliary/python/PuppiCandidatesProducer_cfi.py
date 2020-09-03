import FWCore.ParameterSet.Config as cms

puppi = cms.EDProducer('PuppiCandidatesProducer',
    src = cms.InputTag('packedPFCandidates')
)
