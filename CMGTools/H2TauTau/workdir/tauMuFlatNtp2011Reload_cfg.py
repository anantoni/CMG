import FWCore.ParameterSet.Config as cms
import os

process = cms.Process("FLATNTP")
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )
process.maxLuminosityBlocks = cms.untracked.PSet(input = cms.untracked.int32(-1))
process.options = cms.untracked.PSet( wantSummary = cms.untracked.bool(True) )
evReportFreq = 100

#######Define the sample to process
dataset_user = 'cbern'
sampleTag = "/PAT_CMG_V5_1_0/H2TAUTAU_May7_A"


#########################
sampleName = os.environ['SAMPLENAME']
sampleJobIdx = int(os.environ['SAMPLEJOBIDX'])
process.load('CMGTools.H2TauTau.tools.joseFlatNtpSample_cfi')
from CMGTools.H2TauTau.tools.joseFlatNtpSample2011Reload_cff import configureFlatNtpSample
configureFlatNtpSample(process.flatNtp,sampleName)
#process.flatNtp.diTauTag = cms.InputTag("cmgTauMuCorSVFitFullSel")
process.flatNtp.diTauTag = "cmgTauMuCorSVFitFullSel"
inputfiles = "tauMu_fullsel_tree_CMG_.*root"
dataset_name = process.flatNtp.path.value() + sampleTag
firstfile = sampleJobIdx * 10 
lastfile = (sampleJobIdx + 1 )*10

print dataset_user
print dataset_name
print inputfiles
print firstfile
print lastfile


#get input files
from CMGTools.Production.datasetToSource import *
process.source = datasetToSource( dataset_user, dataset_name, inputfiles)
#process.source.fileNames = cms.untracked.vstring(['/store/cmst3/user/benitezj/CMG/TauPlusX/Run2011A-May10ReReco-v1/AOD/V2/PAT_CMG_V2_4_1/H2TAUTAU_Oct26/h2TauTau_fullsel_tree_CMG_9.root'])
process.source.fileNames = process.source.fileNames[firstfile:lastfile]
print process.source.fileNames


##
process.analysis = cms.Path(process.flatNtp) 
process.schedule = cms.Schedule(process.analysis)
process.TFileService = cms.Service("TFileService", fileName = cms.string("flatNtp.root"))


#####################################################
process.MessageLogger = cms.Service("MessageLogger",
    cerr = cms.untracked.PSet(
    FwkReport = cms.untracked.PSet(
    reportEvery = cms.untracked.int32(evReportFreq),
    optionalPSet = cms.untracked.bool(True),
    limit = cms.untracked.int32(10000000)
    ),
    optionalPSet = cms.untracked.bool(True),
    FwkJob = cms.untracked.PSet(
    optionalPSet = cms.untracked.bool(True),
    limit = cms.untracked.int32(0)
    ),    
    )
)

#process.source.duplicateCheckMode = cms.untracked.string("noDuplicateCheck")
#print process.dumpPython()
