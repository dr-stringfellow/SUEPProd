from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.register('config', default = '', mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.string, info = 'Single-switch config. Values: 03Feb2017, 23Sep2016, Spring16, Summer16')
options.register('globaltag', default = '', mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.string, info = 'Global tag')
options.register('connect', default = '', mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.string, info = 'Globaltag connect')
options.register('printLevel', default = 0, mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.int, info = 'Debug level of the ntuplizer')
options.register('skipEvents', default = 0, mult = VarParsing.multiplicity.singleton, mytype = VarParsing.varType.int, info = 'Skip first events')
options._tags.pop('numEvent%d')
options._tagOrder.remove('numEvent%d')

options.parseArguments()

# Global tags
# https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideFrontierConditions#Global_Tags_for_2017_data_taking
# https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideFrontierConditions#Global_Tags_for_PdmVMCcampaignPh

if options.config == 'Fall17':
    options.globaltag = '94X_mc2017_realistic_v13'
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
for cat in ['SUEPProducer']:
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

##############
## SERVICES ##
##############

process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_condDBv2_cff')
process.GlobalTag.globaltag = options.globaltag

#################
## JET FLAVORS ##
#################

process.load('PhysicsTools.JetMCAlgos.HadronAndPartonSelector_cfi')
from PhysicsTools.JetMCAlgos.AK4PFJetsMCFlavourInfos_cfi import ak4JetFlavourInfos

process.selectedHadronsAndPartons.particles = 'genParticles'

process.ak4GenJetFlavourInfos = ak4JetFlavourInfos.clone(
    jets = 'ak4GenJets'
)
process.ak8GenJetFlavourInfos = ak4JetFlavourInfos.clone(
    jets = 'ak8GenJets',
    rParam = 0.8
)

genJetFlavorSequence = cms.Sequence(
    process.selectedHadronsAndPartons +
    process.ak4GenJetFlavourInfos +
    process.ak8GenJetFlavourInfos
)

### RECO PATH

process.reco = cms.Path(genJetFlavorSequence)

#############
## NTUPLES ##
#############

process.load('SUEPProd.Producer.suep_cfi')
process.suep.isRealData = False
process.suep.useTrigger = False

for fname, conf in process.suep.fillers.parameters_().items():
    if fname not in ['common', 'weights', 'partons', 'genParticles', 'ak4GenJets', 'ak8GenJets', 'extraMets']:
        getattr(process.suep.fillers, fname).enabled = False

process.suep.fillers.common.genParticles = 'genParticles'
del process.suep.fillers.common.finalStateParticles
process.suep.fillers.genParticles.outputMode = 1
process.suep.fillers.ak4GenJets.genJets = 'ak4GenJets'
process.suep.fillers.ak8GenJets.genJets = 'ak8GenJets'
del process.suep.fillers.extraMets.patMet
del process.suep.fillers.extraMets.noHFMet
process.suep.fillers.extraMets.genMet = cms.untracked.string('genMetTrue')
process.suep.fillers.extraMets.types = ['gen']

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
