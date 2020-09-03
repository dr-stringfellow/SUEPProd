from .opts import options

# Global tags
# https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideFrontierConditions

if options.config == '31Mar2018':
    # re-miniaod of 2017 legacy rereco
    options.isData = True
    options.globaltag = '94X_dataRun2_ReReco_EOY17_v6'
    options.redojec = True
elif options.config == '2018Prompt':
    options.isData = True
    options.globaltag = '101X_dataRun2_Prompt_v11'
elif options.config == 'Fall17':
    options.isData = False
    options.globaltag = '94X_mc2017_realistic_v14'
    options.pdfname = 'NNPDF3.1'
    options.redojec = True
elif options.config == 'Summer16':
    options.isData = False
    options.globaltag = '80X_mcRun2_asymptotic_2016_TrancheIV_v8'
    options.redojec = True
elif options.config == 'Autumn18':
    options.isData = False
    options.globaltag = '102X_upgrade2018_realistic_v15'
elif options.config:
    raise RuntimeError('Unknown config ' + options.config)

import FWCore.ParameterSet.Config as cms

process = cms.Process('NTUPLES')

process.options = cms.untracked.PSet(
    numberOfThreads = cms.untracked.uint32(1),
    numberOfStreams = cms.untracked.uint32(0)
)

process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 100
for cat in ['SUEPProducer', 'JetPtMismatchAtLowPt', 'JetPtMismatch', 'NullTransverseMomentum', 'MissingJetConstituent']:
    process.MessageLogger.categories.append(cat)
    setattr(process.MessageLogger.cerr, cat, cms.untracked.PSet(limit = cms.untracked.int32(10)))

############
## SOURCE ##
############

### INPUT FILES
process.source = cms.Source('PoolSource',
    skipEvents = cms.untracked.uint32(options.skipEvents),
    fileNames = cms.untracked.vstring(options.inputFiles)
)

### NUMBER OF EVENTS
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(options.maxEvents)
)

### LUMI MASK
if options.lumilist != '':
    import FWCore.PythonUtilities.LumiList as LumiList
    process.source.lumisToProcess = LumiList.LumiList(filename = options.lumilist).getVLuminosityBlockRange()

##############
## SERVICES ##
##############

process.load('Configuration.Geometry.GeometryRecoDB_cff') 
if not options.isData:
    process.load('Configuration.Geometry.GeometrySimDB_cff')
process.load('Configuration.StandardSequences.Services_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')

process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_condDBv2_cff')
process.GlobalTag.globaltag = options.globaltag

process.RandomNumberGeneratorService.suep = cms.PSet(
    initialSeed = cms.untracked.uint32(1234567),
    engineName = cms.untracked.string('TRandom3')
)
process.RandomNumberGeneratorService.smearedElectrons = cms.PSet(
    initialSeed = cms.untracked.uint32(89101112),
    engineName = cms.untracked.string('TRandom3')
)
process.RandomNumberGeneratorService.smearedPhotons = cms.PSet(
    initialSeed = cms.untracked.uint32(13141516),
    engineName = cms.untracked.string('TRandom3')
)

#############################
## RECO SEQUENCE AND SKIMS ##
#############################

### EGAMMA CORRECTIONS
# https://twiki.cern.ch/twiki/bin/view/CMS/EGMSmearer
   
process.selectedElectrons = cms.EDFilter('PATElectronSelector',
    src = cms.InputTag('slimmedElectrons'),
    cut = cms.string('pt > 5 && abs(eta) < 2.5')
)

from EgammaAnalysis.ElectronTools.calibratedPatElectronsRun2_cfi import calibratedPatElectrons
from EgammaAnalysis.ElectronTools.calibratedPatPhotonsRun2_cfi import calibratedPatPhotons
import EgammaAnalysis.ElectronTools.calibrationTablesRun2
egmSmearingSource = EgammaAnalysis.ElectronTools.calibrationTablesRun2.files
egmSmearingType = 'Run2017_17Nov2017_v1'

process.smearedElectrons = calibratedPatElectrons.clone(
    electrons = 'selectedElectrons',
    isMC = (not options.isData),
    correctionFile = egmSmearingSource[egmSmearingType]
)
process.smearedPhotons = calibratedPatPhotons.clone(
    photons = 'slimmedPhotons',
    isMC = (not options.isData),
    correctionFile = egmSmearingSource[egmSmearingType]
)   

egmCorrectionSequence = cms.Sequence(
    process.selectedElectrons +
    process.smearedElectrons +
    process.smearedPhotons
)

### Vanilla MET
# this is the most basic MET one can find
# even if we override with various types of MET later on, create this so we have a consistent calo MET
# https://twiki.cern.ch/twiki/bin/view/CMS/MissingETUncertaintyPrescription

from PhysicsTools.PatUtils.tools.runMETCorrectionsAndUncertainties import runMetCorAndUncFromMiniAOD

runMetCorAndUncFromMiniAOD(
    process,
    isData = options.isData,
)
metSequence = cms.Sequence(
    process.fullPatMetSequence
)

### PUPPI
from PhysicsTools.PatAlgos.slimming.puppiForMET_cff import makePuppiesFromMiniAOD
## Creates process.puppiMETSequence which includes 'puppi' and 'puppiForMET' (= EDProducer('PuppiPhoton'))
## By default, does not use specific photon ID for PuppiPhoton (which was the case in 80X)
makePuppiesFromMiniAOD(process, createScheduledSequence = True)
## Just renaming
puppiSequence = process.puppiMETSequence

process.puppiNoLep.useExistingWeights = False
process.puppi.useExistingWeights = False

### CHS
process.pfCHS = cms.EDFilter('CandPtrSelector',
    src = cms.InputTag('packedPFCandidates'),
    cut = cms.string('fromPV')
)

### EGAMMA ID
# https://twiki.cern.ch/twiki/bin/view/CMS/EgammaIDRecipesRun2
# https://twiki.cern.ch/twiki/bin/view/CMS/CutBasedElectronIdentificationRun2
# https://twiki.cern.ch/twiki/bin/view/CMS/CutBasedPhotonIdentificationRun2

electronIdParams = {
    'vetoId': 'egmGsfElectronIDs:cutBasedElectronID-Fall17-94X-V1-veto',
    'looseId': 'egmGsfElectronIDs:cutBasedElectronID-Fall17-94X-V1-loose',
    'mediumId': 'egmGsfElectronIDs:cutBasedElectronID-Fall17-94X-V1-medium',
    'tightId': 'egmGsfElectronIDs:cutBasedElectronID-Fall17-94X-V1-tight',
    'mvaWP90': 'egmGsfElectronIDs:mvaEleID-Fall17-noIso-V1-wp90',
    'mvaWP80': 'egmGsfElectronIDs:mvaEleID-Fall17-noIso-V1-wp80',
    'mvaWPLoose': 'egmGsfElectronIDs:mvaEleID-Fall17-noIso-V1-wpLoose',
    'mvaIsoWP90': 'egmGsfElectronIDs:mvaEleID-Fall17-iso-V1-wp90',
    'mvaIsoWP80': 'egmGsfElectronIDs:mvaEleID-Fall17-iso-V1-wp80',
    'mvaIsoWPLoose': 'egmGsfElectronIDs:mvaEleID-Fall17-iso-V1-wpLoose',
    'hltId': 'egmGsfElectronIDs:cutBasedElectronHLTPreselection-Summer16-V1', # seems like we don't have these for >= 2017?
    'mvaValuesMap': 'electronMVAValueMapProducer:ElectronMVAEstimatorRun2Spring16GeneralPurposeV1Values',
    #'mvaCategoriesMap': 'electronMVAValueMapProducer:ElectronMVAEstimatorRun2Spring16GeneralPurposeV1Categories',
    'combIsoEA': 'RecoEgamma/ElectronIdentification/data/Fall17/effAreaElectrons_cone03_pfNeuHadronsAndPhotons_92X.txt',
    'ecalIsoEA': 'RecoEgamma/ElectronIdentification/data/Summer16/effAreaElectrons_HLT_ecalPFClusterIso.txt',
    'hcalIsoEA': 'RecoEgamma/ElectronIdentification/data/Summer16/effAreaElectrons_HLT_hcalPFClusterIso.txt'
}

photonIdParams = {
    'looseId': 'egmPhotonIDs:cutBasedPhotonID-Fall17-94X-V1-loose',
    'mediumId': 'egmPhotonIDs:cutBasedPhotonID-Fall17-94X-V1-medium',
    'tightId': 'egmPhotonIDs:cutBasedPhotonID-Fall17-94X-V1-tight',
    'chIsoEA': 'RecoEgamma/PhotonIdentification/data/Fall17/effAreaPhotons_cone03_pfChargedHadrons_90percentBased_TrueVtx.txt',
    'nhIsoEA': 'RecoEgamma/PhotonIdentification/data/Fall17/effAreaPhotons_cone03_pfNeutralHadrons_90percentBased_TrueVtx.txt',
    'phIsoEA': 'RecoEgamma/PhotonIdentification/data/Fall17/effAreaPhotons_cone03_pfPhotons_90percentBased_TrueVtx.txt'
}

electronIdModules = [
    'RecoEgamma.ElectronIdentification.Identification.mvaElectronID_Fall17_noIso_V1_cff',
    'RecoEgamma.ElectronIdentification.Identification.mvaElectronID_Fall17_iso_V1_cff',
    'RecoEgamma.ElectronIdentification.Identification.cutBasedElectronID_Fall17_94X_V1_cff',
    'RecoEgamma.ElectronIdentification.Identification.cutBasedElectronHLTPreselecition_Summer16_V1_cff'
]

photonIdModules = [
    'RecoEgamma.PhotonIdentification.Identification.cutBasedPhotonID_Fall17_94X_V1_TrueVtx_cff'
]

from PhysicsTools.SelectorUtils.tools.vid_id_tools import setupAllVIDIdsInModule, setupVIDElectronSelection, setupVIDPhotonSelection, switchOnVIDElectronIdProducer, switchOnVIDPhotonIdProducer, DataFormat
# Loads egmGsfElectronIDs
switchOnVIDElectronIdProducer(process, DataFormat.MiniAOD)
for idmod in electronIdModules:
    setupAllVIDIdsInModule(process, idmod, setupVIDElectronSelection)

switchOnVIDPhotonIdProducer(process, DataFormat.MiniAOD)
for idmod in photonIdModules:
    setupAllVIDIdsInModule(process, idmod, setupVIDPhotonSelection)

process.load('SUEPProd.Auxiliary.WorstIsolationProducer_cfi')

egmIdSequence = cms.Sequence(
    process.photonIDValueMapProducer +
    process.egmPhotonIDs +
    process.electronMVAValueMapProducer +
    process.egmGsfElectronIDs +
    process.worstIsolationProducer
)

### REMAKE CHS JETS WITH DEEP FLAVOR

from SUEPProd.Producer.utils.makeJets_cff import makeJets

slimmedJetsSequence = makeJets(process, options.isData, 'AK4PFchs', 'pfCHS', 'DeepFlavor')

if options.redojec:

    ### JET RE-CORRECTION
    from PhysicsTools.PatAlgos.producersLayer1.jetUpdater_cff import updatedPatJetCorrFactors, updatedPatJets

    jecLevels= ['L1FastJet',  'L2Relative', 'L3Absolute']
    if options.isData:
        jecLevels.append('L2L3Residual')

    #
    # slimmedJets are made from scratch (makeJets_cff)
    #

    #process.updatedPatJetCorrFactors = updatedPatJetCorrFactors.clone(
    #    src = cms.InputTag('slimmedJets', '', cms.InputTag.skipCurrentProcess()),
    #    levels = cms.vstring(*jecLevels),
    #)
    #
    #process.slimmedJets = updatedPatJets.clone(
    #    jetSource = cms.InputTag('slimmedJets', '', cms.InputTag.skipCurrentProcess()),
    #    addJetCorrFactors = cms.bool(True),
    #    jetCorrFactorsSource = cms.VInputTag(cms.InputTag('updatedPatJetCorrFactors')),
    #    addBTagInfo = cms.bool(False),
    #    addDiscriminators = cms.bool(False)
    #)

    process.updatedPatJetCorrFactorsPuppi = updatedPatJetCorrFactors.clone(
        src = cms.InputTag('slimmedJetsPuppi', '', cms.InputTag.skipCurrentProcess()),
        levels = cms.vstring(*jecLevels),
    )

    process.slimmedJetsPuppi = updatedPatJets.clone(
        jetSource = cms.InputTag('slimmedJetsPuppi', '', cms.InputTag.skipCurrentProcess()),
        addJetCorrFactors = cms.bool(True),
        jetCorrFactorsSource = cms.VInputTag(cms.InputTag('updatedPatJetCorrFactorsPuppi')),
        addBTagInfo = cms.bool(False),
        addDiscriminators = cms.bool(False)
    )

    ### MET RE-CORRECTION
    # pfMet is already corrected above in the VANILLA MET section

    runMetCorAndUncFromMiniAOD(
        process,
        isData = options.isData,
        metType = "Puppi",
        postfix = "Puppi",
        jetFlavor = "AK4PFPuppi"
    )

    jetRecorrectionSequence = cms.Sequence(
        #process.updatedPatJetCorrFactors +
        #process.slimmedJets +
        process.updatedPatJetCorrFactorsPuppi +
        process.slimmedJetsPuppi +
        process.fullPatMetSequencePuppi
    )

else:
    jetRecorrectionSequence = cms.Sequence()

### FAT JETS

from SUEPProd.Producer.utils.makeFatJets_cff import initFatJets, makeFatJets

fatJetInitSequence = initFatJets(process, options.isData, ['AK8', 'CA15'])

ak8CHSSequence = makeFatJets(
    process,
    isData = options.isData,
    label = 'AK8PFchs',
    candidates = 'pfCHS'
)

ak8PuppiSequence = makeFatJets(
    process,
    isData = options.isData,
    label = 'AK8PFPuppi',
    candidates = 'puppi'
)

ca15PuppiSequence = makeFatJets(
    process,
    isData = options.isData,
    label = 'CA15PFPuppi',
    candidates = 'puppi'
)

fatJetSequence = cms.Sequence(
    fatJetInitSequence +
    ak8CHSSequence +
    ak8PuppiSequence +
    ca15PuppiSequence
)

### MERGE GEN PARTICLES
do_merge = not options.isData and False
if do_merge:
    process.load('SUEPProd.Auxiliary.MergedGenProducer_cfi')
    genMergeSequence = cms.Sequence( process.mergedGenParticles )
else:
    genMergeSequence = cms.Sequence()

### GEN JET FLAVORS
if not options.isData:
    process.load('PhysicsTools.JetMCAlgos.HadronAndPartonSelector_cfi')
    from PhysicsTools.JetMCAlgos.HadronAndPartonSelector_cfi import selectedHadronsAndPartons
    from PhysicsTools.JetMCAlgos.GenHFHadronMatcher_cff import matchGenBHadron
    from PhysicsTools.JetMCAlgos.GenHFHadronMatcher_cff import matchGenCHadron
    from PhysicsTools.JetMCAlgos.AK4PFJetsMCFlavourInfos_cfi import ak4JetFlavourInfos
    # Input particle collection for matching to gen jets (partons + leptons) 
    # MUST use use proper input jet collection: the jets to which hadrons should be associated
    # rParam and jetAlgorithm MUST match those used for jets to be associated with hadrons
    # More details on the tool: https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideBTagMCTools#New_jet_flavour_definition

    process.selectedHadronsAndPartons.particles = 'mergedGenParticles' if do_merge else 'prunedGenParticles'

    process.ak4GenJetFlavourInfos = ak4JetFlavourInfos.clone(
        jets = 'slimmedGenJets'
    )
    process.ak8GenJetFlavourInfos = ak4JetFlavourInfos.clone(
        jets = 'genJetsNoNuAK8',
        rParam = 0.8
    )
    process.ca15GenJetFlavourInfos = ak4JetFlavourInfos.clone(
        jets = 'genJetsNoNuCA15',
        jetAlgorithm = 'CambridgeAachen',
        rParam = 1.5
    )
    
    # Begin GenHFHadronMatcher subsequences
    # Adapted from PhysicsTools/JetMCAlgos/test/matchGenHFHadrons.py
    # Supplies PDG ID to real name resolution of MC particles
    process.load("SimGeneral.HepPDTESSource.pythiapdt_cfi")
    
    process.ak4MatchGenBHadron = matchGenBHadron.clone(
        genParticles = process.selectedHadronsAndPartons.particles,
        jetFlavourInfos = "ak4GenJetFlavourInfos"
    )
    process.ak4MatchGenCHadron = matchGenCHadron.clone(
        genParticles = process.selectedHadronsAndPartons.particles,
        jetFlavourInfos = "ak4GenJetFlavourInfos"
    )
    process.ak8MatchGenBHadron = matchGenBHadron.clone(
        genParticles = process.selectedHadronsAndPartons.particles,
        jetFlavourInfos = "ak8GenJetFlavourInfos"
    )
    process.ak8MatchGenCHadron = matchGenCHadron.clone(
        genParticles = process.selectedHadronsAndPartons.particles,
        jetFlavourInfos = "ak8GenJetFlavourInfos"
    )
    process.ca15MatchGenBHadron = matchGenBHadron.clone(
        genParticles = process.selectedHadronsAndPartons.particles,
        jetFlavourInfos = "ca15GenJetFlavourInfos"
    )
    process.ca15MatchGenCHadron = matchGenCHadron.clone(
        genParticles = process.selectedHadronsAndPartons.particles,
        jetFlavourInfos = "ca15GenJetFlavourInfos"
    )
    #End GenHFHadronMatcher subsequences

    genJetFlavorSequence = cms.Sequence(
        process.selectedHadronsAndPartons +
        process.ak4GenJetFlavourInfos +
        process.ak8GenJetFlavourInfos +
        process.ca15GenJetFlavourInfos + 
        process.ak4MatchGenBHadron +
        process.ak4MatchGenCHadron +
        process.ak8MatchGenBHadron +
        process.ak8MatchGenCHadron +
        process.ca15MatchGenBHadron +
        process.ca15MatchGenCHadron
    )
else:
    genJetFlavorSequence = cms.Sequence()

# runMetCorAnd.. adds a CaloMET module only once, adding the postfix
# However, repeated calls to the function overwrites the MET source of patCaloMet
process.patCaloMet.metSource = 'metrawCalo'

### MONOX FILTER

process.load('SUEPProd.Filters.MonoXFilter_cfi')
process.MonoXFilter.taggingMode = True

### RECO PATH

process.reco = cms.Path(
    egmCorrectionSequence +
    egmIdSequence +
    puppiSequence +
    metSequence +
    slimmedJetsSequence +
    jetRecorrectionSequence +
    process.MonoXFilter +
    fatJetSequence +
    genMergeSequence + 
    genJetFlavorSequence
)

#############
## NTULPES ##
#############

process.load('SUEPProd.Producer.suep_cfi')
process.suep.isRealData = options.isData
process.suep.useTrigger = options.useTrigger
#process.suep.SelectEvents = ['reco'] # no skim
process.suep.fillers.chsAK4Jets.jets = 'slimmedJetsDeepFlavor'

if options.isData:
    process.suep.fillers.partons.enabled = False
    process.suep.fillers.genParticles.enabled = False
    process.suep.fillers.ak4GenJets.enabled = False
    process.suep.fillers.ak8GenJets.enabled = False
    process.suep.fillers.ca15GenJets.enabled = False
else:
    process.suep.fillers.weights.pdfType = options.pdfname
    process.suep.fillers.extraMets.types.append('gen')

if not options.useTrigger:
    process.suep.fillers.hlt.enabled = False

for name, value in electronIdParams.items():
    setattr(process.suep.fillers.electrons, name, value)

for name, value in photonIdParams.items():
    setattr(process.suep.fillers.photons, name, value)

process.suep.fillers.muons.rochesterCorrectionSource = 'SUEPProd/Utilities/data/RoccoR2017v0.txt'

process.suep.outputFile = options.outputFile
process.suep.printLevel = options.printLevel

process.ntuples = cms.EndPath(process.suep)

##############
## SCHEDULE ##
##############

process.schedule = cms.Schedule(process.reco, process.ntuples)

if options.connect:
    if options.connect == 'mit':
        options.connect = 'frontier://(proxyurl=http://squid.cmsaf.mit.edu:3128)(proxyurl=http://squid1.cmsaf.mit.edu:3128)(proxyurl=http://squid2.cmsaf.mit.edu:3128)(serverurl=http://cmsfrontier.cern.ch:8000/FrontierProd)/CMS_CONDITIONS'

    process.GlobalTag.connect = options.connect
    for toGet in process.GlobalTag.toGet:
        toGet.connect = options.connect

if options.dumpPython:
    print process.dumpPython()
