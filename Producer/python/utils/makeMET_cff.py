import FWCore.ParameterSet.Config as cms
import RecoMET.METProducers.PFMET_cfi as PFMET_cfi
from RecoMET.METProducers.METSignificanceParams_cfi import METSignificanceParams, METSignificanceParams_Data
import PhysicsTools.PatUtils.patPFMETCorrections_cff as patMET_cff
from PhysicsTools.PatAlgos.producersLayer1.metProducer_cfi import patMETs
from PhysicsTools.PatAlgos.selectionLayer1.jetSelector_cfi import selectedPatJets
from PhysicsTools.PatAlgos.cleaningLayer1.jetCleaner_cfi import cleanPatJets
from PhysicsTools.PatAlgos.slimming.slimmedMETs_cfi import slimmedMETs

from SUEPProd.Producer.utils.addattr import AddAttr

electrons = cms.InputTag('slimmedElectrons')
muons = cms.InputTag('slimmedMuons')
taus = cms.InputTag('slimmedTaus')
photons = cms.InputTag('slimmedPhotons')

def initMET(process, isData):
    """
    Parts of MET re-reconstruction that are independent of PF candidate collection.
    """

    sequence = cms.Sequence()

    addattr = AddAttr(process, sequence)
  
    addattr('caloMet',
        cms.EDProducer("RecoMETExtractor",
            metSource = cms.InputTag("slimmedMETs", processName = cms.InputTag.skipCurrentProcess()),
            correctionLevel = cms.string('rawCalo')
        )
    )

    if not isData:
        addattr('genMetTrue',
            cms.EDProducer("GenMETExtractor",
                metSource= cms.InputTag("slimmedMETs", processName = cms.InputTag.skipCurrentProcess())
            )
        )

    addattr('patCaloMet',
        patMETs.clone(
            metSource = 'caloMet',
            addMuonCorrections = False,
            addGenMET = False
        )
    )

    return sequence

def makeMET(process, isData, pfCandidates, jetSource, jetFlavor, postfix = ''):
    """
    @jetFlavor: e.g. 'ak4PFchs'

    Additional information (such as gen and calo mets) are added only if postfix is empty.
    """

    sequence = cms.Sequence()
    # postfix is automatically added to the module names
    addattr = AddAttr(process, sequence, postfix)

    if postfix == '':
        # default MET - extract from input slimmedMETs
        pfMet = addattr('pfMet',
            cms.EDProducer("RecoMETExtractor",
                metSource = cms.InputTag("slimmedMETs", processName = cms.InputTag.skipCurrentProcess()),
                correctionLevel = cms.string('raw')
            )
        )
    else:
        pfMet = addattr('pfMet',
            PFMET_cfi.pfMet.clone(
                src = pfCandidates,
                calculateSignificance = False # done in PAT
            )
        )

    cleanedJets = addattr('cleanedJetsForMET',
        cms.EDProducer("PATJetCleanerForType1MET",
            src = cms.InputTag(jetSource),
            jetCorrEtaMax = cms.double(9.9),
            jetCorrLabel = cms.InputTag("L3Absolute"),
            jetCorrLabelRes = cms.InputTag("L2L3Residual"),
            offsetCorrLabel = cms.InputTag("L1FastJet"),
            skipEM = cms.bool(True),
            skipEMfractionThreshold = cms.double(0.9),
            skipMuonSelection = cms.string('isGlobalMuon | isStandAloneMuon'),
            skipMuons = cms.bool(True),
            type1JetPtThreshold = cms.double(15.0)
        )
    )
    
    selectedJets = addattr('selectedJetsForMET',
        selectedPatJets.clone(
            src = cleanedJets,
            cut = 'pt > 15 && abs(eta) < 9.9'
        )
    )
    
    crossCleanedJets = addattr('crossCleanedJetsForMET',
        cleanPatJets.clone(
            src = selectedJets
        )
    )
    ccJetsMod = addattr.last
    ccJetsMod.checkOverlaps.muons.src = muons
    ccJetsMod.checkOverlaps.electrons.src = electrons
    del ccJetsMod.checkOverlaps.photons
    del ccJetsMod.checkOverlaps.taus
    # not used at all and electrons are already cleaned
    del ccJetsMod.checkOverlaps.tkIsoElectrons

    patPFMet = addattr('patPFMet',
        patMET_cff.patPFMet.clone(
            metSource = pfMet,
            genMETSource = 'genMetTrue',
            srcPFCands = pfCandidates,
            computeMETSignificance = True,
            parameters = (METSignificanceParams_Data if isData else METSignificanceParams),
            srcJets = crossCleanedJets,
            srcLeptons = [electrons, muons, photons],
            addGenMET = (not isData and postfix == '')
        )
    )

    patPFMetT1Corr = addattr('patPFMetT1Corr',
        patMET_cff.patPFMetT1T2Corr.clone(
            src = crossCleanedJets
        )
    )
    
    patPFMetT1 = addattr('patPFMetT1',
        patMET_cff.patPFMetT1.clone(
            src = patPFMet,
            srcCorrections = [cms.InputTag(patPFMetT1Corr.getModuleLabel(), 'type1')]
        )
    )
    
    pfCandsNoEle = addattr('pfCandsNoEle',
        cms.EDProducer("CandPtrProjector", 
            src = cms.InputTag(pfCandidates),
            veto = electrons
        )
    )

    pfCandsNoEleMu = addattr('pfCandsNoEleMu',
        cms.EDProducer("CandPtrProjector", 
            src = pfCandsNoEle,
            veto = muons
        )
    )
        
    pfCandsNoEleMuTau = addattr('pfCandsNoEleMuTau',
        cms.EDProducer("CandPtrProjector", 
            src = pfCandsNoEleMu,
            veto = taus
        )
    )
        
    pfCandsNoEleMuTauGamma = addattr('pfCandsNoEleMuTauGamma',
        cms.EDProducer("CandPtrProjector", 
            src = pfCandsNoEleMuTau,
            veto = photons
        )
    )
    
    pfCandsForUnclusteredUnc = addattr('pfCandsForUnclusteredUnc',
        cms.EDProducer("CandPtrProjector", 
            src = pfCandsNoEleMuTauGamma,
            veto = crossCleanedJets
        )
    )
    
    for vsign, vname in [(1, 'Up'), (-1, 'Down')]:
        shiftConf = [
            ('MuonEn', muons.value(), '((x<100)?(0.002+0*y):(0.05+0*y))'),
            ('ElectronEn', electrons.value(), '((abs(y)<1.479)?(0.006+0*x):(0.015+0*x))'),
            ('PhotonEn', photons.value(), '((abs(y)<1.479)?(0.01+0*x):(0.025+0*x))'),
            ('TauEn', taus.value(), '0.03+0*x*y'),
            ('UnclusteredEn', pfCandsForUnclusteredUnc.value(), ''),
            ('JetEn', crossCleanedJets.value(), '')
        ]
    
        for part, coll, formula in shiftConf:
            if part == 'UnclusteredEn':
                shifted = addattr('shifted' + part + vname,
                    cms.EDProducer("ShiftedParticleProducer",
                        src = pfCandsForUnclusteredUnc,
                        binning = cms.VPSet(
                            # charged PF hadrons - tracker resolution
                            cms.PSet(
                                binSelection = cms.string('charge!=0'),
                                binUncertainty = cms.string('sqrt(pow(0.00009*x,2)+pow(0.0085/sqrt(sin(2*atan(exp(-y)))),2))')
                            ),
                            # neutral PF hadrons - HCAL resolution
                            cms.PSet(
                                binSelection = cms.string('pdgId==130'),
                                energyDependency = cms.bool(True),
                                binUncertainty = cms.string('((abs(y)<1.3)?(min(0.25,sqrt(0.64/x+0.0025))):(min(0.30,sqrt(1.0/x+0.0016))))')
                            ),
                            # photon - ECAL resolution
                            cms.PSet(
                                binSelection = cms.string('pdgId==22'),
                                energyDependency = cms.bool(True),
                                binUncertainty = cms.string('sqrt(0.0009/x+0.000001)+0*y')
                            ),
                            # HF particules - HF resolution
                            cms.PSet(
                                binSelection = cms.string('pdgId==1 || pdgId==2'),
                                energyDependency = cms.bool(True),
                                binUncertainty = cms.string('sqrt(1./x+0.0025)+0*y')
                            ),
                        ),
                        shiftBy = cms.double(float(vsign))
                    )
                )

            elif part == 'JetEn':
                shifted = addattr('shifted' + part + vname,
                    cms.EDProducer('SUEPShiftedPATJetProducer',
                        src = crossCleanedJets,
                        jetCorrPayloadName =  cms.string(jetFlavor),
                        jetCorrUncertaintyTag = cms.string('Uncertainty'),
                        addResidualJES = cms.bool(isData),
                        jetCorrLabelUpToL3 = cms.InputTag('L3Absolute'), # use embedded correction factors
                        jetCorrLabelUpToL3Res = cms.InputTag('L2L3Residual'),
                        shiftBy = cms.double(float(vsign))
                    )
                )

            else:
                shifted = addattr('shifted' + part + vname,
                    cms.EDProducer("ShiftedParticleProducer",
                        src = cms.InputTag(coll),
                        uncertainty = cms.string(formula),
                        shiftBy = cms.double(float(vsign))
                    )
                )

            metCorrShifted = addattr('metCorrShifted' + part + vname,
                cms.EDProducer("ShiftedParticleMETcorrInputProducer",
                    srcOriginal = cms.InputTag(coll),
                    srcShifted = shifted
                )
            )
            addattr('patPFMetT1' + part + vname,
                patMET_cff.patPFMetT1.clone(
                    src = patPFMetT1,
                    srcCorrections = [metCorrShifted]
                )
            )

    # Dummy JetResUp and JetResDown modules because PATJetSlimmer requires them
    # Jet smearing should be propagated to MET simply by using ptSmear(|Up|Down) branches at the ntuples level
    addattr('patPFMetT1JetResUp', getattr(process, 'patPFMetT1JetEnUp' + postfix).clone())
    addattr('patPFMetT1JetResDown', getattr(process, 'patPFMetT1JetEnDown' + postfix).clone())
    
    addattr('slimmedMETs',
        slimmedMETs.clone(
            src = patPFMetT1,
            rawVariation = patPFMet,
            t1Uncertainties = "patPFMetT1%s" + postfix,
            runningOnMiniAOD = True
        )
    )

    slimmed = addattr.last

    if postfix == '': # default MET
        slimmed.caloMET = 'patCaloMet'
    else:
        del slimmed.caloMET
    
    del slimmed.t01Variation
    del slimmed.t1SmearedVarsAndUncs
    del slimmed.tXYUncForT1
    del slimmed.tXYUncForRaw
    del slimmed.tXYUncForT01
    del slimmed.tXYUncForT1Smear
    del slimmed.tXYUncForT01Smear

    return sequence
