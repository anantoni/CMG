import FWCore.ParameterSet.Config as cms

from Configuration.Generator.PythiaUEZ2starSettings_cfi import pythiaUESettingsBlock as Z2starSettings

generator = cms.EDProducer("Pythia6PtYDistGun",
                           maxEventsToPrint = cms.untracked.int32(5),
                           pythiaHepMCVerbosity = cms.untracked.bool(True),
                           pythiaPylistVerbosity = cms.untracked.int32(1),
                           PGunParameters = cms.PSet(  ParticleID = cms.vint32(21),
                                                       kinematicsFile = cms.FileInPath('CMG/JetIDAnalysis/data/ptgun.root'),
                                                       PtBinning = cms.int32(100000),
                                                       YBinning = cms.int32(500),
                                                       MinPt = cms.double(3.0),
                                                       MaxPt = cms.double(300.0),
                                                       MinY = cms.double(-5.0),
                                                       MaxY = cms.double(5.0),
                                                       MinPhi = cms.double(-3.14159265359),
                                                       MaxPhi = cms.double(3.14159265359),
                                                       ),
                           Verbosity = cms.untracked.int32(0),
                           psethack = cms.string('single gluon pt 3-300'),
                           firstRun = cms.untracked.uint32(1),
                           PythiaParameters = cms.PSet(  pythiaUESettings = Z2starSettings.pythiaUESettings,
                                                         parameterSets = cms.vstring('pythiaUESettings')
                                                         )
                           )
