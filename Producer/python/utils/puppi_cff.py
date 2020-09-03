import FWCore.ParameterSet.Config as cms

# Copied from PhysicsTools.PatAlgos.slimming.puppiForMET_cff import makePuppiesFromMiniAOD

from CommonTools.PileupAlgos.Puppi_cff import puppi
puppi.candName = 'packedPFCandidates'
puppi.vertexName = 'offlineSlimmedPrimaryVertices'
#puppi.useExistingWeights = False # I still don't trust miniaod...
puppi.useExistingWeights = True
puppi.clonePackedCands = True # if !useExistingWeights, need to set this flag to make PuppiProducer create packed candidates

pfNoLepPUPPI = cms.EDFilter("CandPtrSelector",
    src = cms.InputTag("packedPFCandidates"),
    cut = cms.string("abs(pdgId) != 13 && abs(pdgId) != 11 && abs(pdgId) != 15")
)
puppiNoLep = puppi.clone(
    candName = 'pfNoLepPUPPI',
    useWeightsNoLep = True,
    useExistingWeights = False
)

pfLeptonsPUPPET = cms.EDFilter("CandPtrSelector",
    src = cms.InputTag("packedPFCandidates"),
    cut = cms.string("abs(pdgId) == 13 || abs(pdgId) == 11 || abs(pdgId) == 15")
)

puppiMerged = cms.EDProducer("CandViewMerger",
    src = cms.VInputTag('puppiNoLep', 'pfLeptonsPUPPET')
)

import SUEPProd.Producer.utils.egmidconf as egmidconf

from CommonTools.PileupAlgos.PhotonPuppi_cff import puppiPhoton
puppiForMET = puppiPhoton.clone(
    candName = 'packedPFCandidates',
    photonName = 'slimmedPhotons',
    runOnMiniAOD = True,
    puppiCandName = 'puppiMerged',
    useRefs = False, # need to perform dR matching because of "an issue in refs in PackedCandidates"
    photonId  = egmidconf.photonLooseId
)

puppiMETSequence = cms.Sequence(
    puppi +
    pfNoLepPUPPI +
    pfLeptonsPUPPET +
    puppiNoLep +
    puppiMerged +
    puppiForMET
)
