import FWCore.ParameterSet.Config as cms

mvaMETTauEle = cms.EDProducer(
    "MVAMETProducerTauEle",
    pfmetSrc = cms.InputTag('pfMetForRegression'),
    tkmetSrc = cms.InputTag('tkMet'),
    nopumetSrc = cms.InputTag('nopuMet'),
    pucmetSrc = cms.InputTag('pcMet'),
    pumetSrc = cms.InputTag('puMet'),
    recBosonSrc = cms.InputTag('cmgTauEleSel'),
    jetSrc = cms.InputTag('cmgPFJetSel'),
    leadJetSrc = cms.InputTag('cmgPFBaseJetLead'),
    vertexSrc = cms.InputTag('goodPVFilter'),
    nJetsPtGt1Src = cms.InputTag('nJetsPtGt1'),
    rhoSrc = cms.InputTag('kt6PFJets','rho'),
    enable = cms.bool(True),
    verbose = cms.untracked.bool( False )
    #COLIN: make delta R a parameter
    )
