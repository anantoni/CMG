import FWCore.ParameterSet.Config as cms

from CMGTools.Common.selections.btaggedjet_cfi import trackCountingHighEffBJetTags
from CMGTools.Common.selections.kinematics_cfi import kinematics

baseJetFactory = cms.PSet(
       inputCollection = cms.InputTag("selectedPatJetsAK5"),
       btagType = cms.string('trackCountingHighEffBJetTags'),
       applyJecUncertainty = cms.bool(False),
       jecPath = cms.string("CondFormats/JetMETObjects/data/Spring10_Uncertainty_AK5Calo.txt"),
       jecUncDirection = cms.int32(1)
       )

cmgBaseJet = cms.EDFilter(
    "BaseJetPOProducer",
    cfg = baseJetFactory.clone(),
    cuts = cms.PSet(
       btag = trackCountingHighEffBJetTags.clone(),
       jetKinematics = kinematics.clone()
       )
)
