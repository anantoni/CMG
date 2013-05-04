import FWCore.ParameterSet.Config as cms

from CMGTools.External.pujetidproducer_cfi import pileupJetIdProducerChs

dataAnalyzer = cms.EDAnalyzer( "DataAnalyzer",
                               cfg=cms.PSet(
                               triggerSource = cms.InputTag("TriggerResults::HLT"),
                               triggerPaths = cms.vstring('Ele17_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_Ele8_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_v',
                                                          'DoubleEle33_CaloIdL_GsfTrkIdVL_v'
                                                          'Mu17_Mu8_v',
                                                          'Mu17_TkMu8_v',
                                                          'Mu8_Ele17_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_v',
                                                          'Mu17_Ele8_CaloIdT_CaloIsoVL_TrkIdVL_TrkIsoVL_v',
                                                          'Ele27 WP80_v'
                                                          'IsoMu24_eta2p1_v',
                                                          'Photon36_R9Id90_HE10_Iso40_EBOnly_v',                  
                                                          'Photon50_R9Id90_HE10_Iso40_EBOnly_v',
                                                          'Photon75_R9Id90_HE10_Iso40_EBOnly_v',
                                                          'Photon90_R9Id90_HE10_Iso40_EBOnly_v',
                                                          'Photon250_NoHE_v1_v',
                                                          'Photon300_NoHE_v1_v'),
                               triggerCats  = cms.vint32(1111,
                                                         1111,
                                                         1313,
                                                         1313,
                                                         1113,
                                                         1113,
                                                         11,
                                                         13,
                                                         22,
                                                         22,
                                                         22,
                                                         22,
                                                         22,
                                                         22
                                                         ),
                               genSource       = cms.InputTag("genParticles"),
                               vtxSource       = cms.InputTag("goodOfflinePrimaryVertices"),
                               beamSpotSource  = cms.InputTag("offlineBeamSpot"),
                               pfSource        = cms.InputTag("particleFlow"),
                               muonSource      = cms.InputTag("selectedPatMuonsPFlow"),
                               electronSource  = cms.InputTag("selectedPatElectronsPFlow"),
                               photonSource    = cms.InputTag("photons"),
                               conversionSource= cms.InputTag("allConversions"),
                               ebrechitsSource = cms.InputTag("reducedEcalRecHitsEB"),
                               eerechitsSource = cms.InputTag("reducedEcalRecHitsEE"),
                               jetSource       = cms.InputTag("selectedPatJetsPFlow"),
                               pujetidAlgo     = pileupJetIdProducerChs.algos,
                               metSource       = cms.VInputTag("selectedPatMETsPFlow"),
                               rhoSource       = cms.InputTag("kt6PFJets:rho"),
                               rho25Source      = cms.InputTag("kt6PFJetsCentral:rho")
                               )
                               )
