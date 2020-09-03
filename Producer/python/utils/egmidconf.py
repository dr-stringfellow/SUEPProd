# "Common block" loaded by the cfi and the cfg to communicate egmID parameters

electronVetoId = 'egmGsfElectronIDs:cutBasedElectronID-Summer16-80X-V1-veto'
electronLooseId = 'egmGsfElectronIDs:cutBasedElectronID-Summer16-80X-V1-loose'
electronMediumId = 'egmGsfElectronIDs:cutBasedElectronID-Summer16-80X-V1-medium'
electronTightId = 'egmGsfElectronIDs:cutBasedElectronID-Summer16-80X-V1-tight'
electronHLTId = 'egmGsfElectronIDs:cutBasedElectronHLTPreselection-Summer16-V1'
electronMVAWP90 = 'egmGsfElectronIDs:mvaEleID-Spring16-GeneralPurpose-V1-wp90'
electronMVAWP80 = 'egmGsfElectronIDs:mvaEleID-Spring16-GeneralPurpose-V1-wp80'
electronCombIsoEA = 'RecoEgamma/ElectronIdentification/data/Summer16/effAreaElectrons_cone03_pfNeuHadronsAndPhotons_80X.txt'
electronEcalIsoEA = 'RecoEgamma/ElectronIdentification/data/Summer16/effAreaElectrons_HLT_ecalPFClusterIso.txt'
electronHcalIsoEA = 'RecoEgamma/ElectronIdentification/data/Summer16/effAreaElectrons_HLT_hcalPFClusterIso.txt'

photonLooseId = 'egmPhotonIDs:cutBasedPhotonID-Spring16-V2p2-loose'
photonMediumId = 'egmPhotonIDs:cutBasedPhotonID-Spring16-V2p2-medium'
photonTightId = 'egmPhotonIDs:cutBasedPhotonID-Spring16-V2p2-tight'
photonCHIsoEA = 'RecoEgamma/PhotonIdentification/data/Spring16/effAreaPhotons_cone03_pfChargedHadrons_90percentBased.txt'
photonNHIsoEA = 'RecoEgamma/PhotonIdentification/data/Spring16/effAreaPhotons_cone03_pfNeutralHadrons_90percentBased.txt'
photonPhIsoEA = 'RecoEgamma/PhotonIdentification/data/Spring16/effAreaPhotons_cone03_pfPhotons_90percentBased.txt'

# https://twiki.cern.ch/twiki/bin/view/CMS/EGMSmearer#Energy_smearing_and_scale_correc
# https://github.com/ECALELFS/ScalesSmearings/tree/Moriond17_23Jan_v2

electronSmearingData = {
    "Prompt2015":"SUEPProd/Producer/data/ScalesSmearings/74X_Prompt_2015",
    "76XReReco" :"SUEPProd/Producer/data/ScalesSmearings/76X_16DecRereco_2015_Etunc",
    "80Xapproval" : "SUEPProd/Producer/data/ScalesSmearings/80X_ichepV1_2016_ele",
    "Moriond2017_JEC" : "SUEPProd/Producer/data/ScalesSmearings/Winter_2016_reReco_v1_ele"     #only to derive JEC corrections
}

photonSmearingData = {
    "Prompt2015":"SUEPProd/Producer/data/ScalesSmearings/74X_Prompt_2015",
    "76XReReco" :"SUEPProd/Producer/data/ScalesSmearings/76X_16DecRereco_2015_Etunc",
    "80Xapproval" : "SUEPProd/Producer/data/ScalesSmearings/80X_ichepV2_2016_pho",
    "Moriond2017_JEC" : "SUEPProd/Producer/data/ScalesSmearings/Winter_2016_reReco_v1_ele"     #only to derive JEC correctionsb
}
