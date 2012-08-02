from PhysicsTools.PatAlgos.patTemplate_cfg import *

##########


process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )

process.maxLuminosityBlocks = cms.untracked.PSet( 
    input = cms.untracked.int32(-1)
    )


##########

# Message logger setup.
process.MessageLogger.cerr.FwkReport.reportEvery = 100
process.options = cms.untracked.PSet( wantSummary = cms.untracked.bool(True) )

process.setName_('SUSY')

#COLIN: tell Will to remove the rather printouts (or rather to add an enable flag allowing to get these printouts). What are these printouts? 

# process.load("CMGTools.Common.sources.HT.Run2011A_PromptReco_v1.AOD.PAT_CMG.source_tree_cff")
# process.load("CMGTools.Common.sources.HT.Run2011A_May10ReReco_v1.AOD.PAT_CMG.source_cff")
# process.load("CMGTools.Common.sources.HT.Run2011A_May10ReReco_v1.AOD.PAT_CMG_NewJEC.source_cff")
# process.load("CMGTools.Common.sources.HT.Run2011A_HighMET_PromptSkim_v4.RAW_RECO.PAT_CMG_NewJEC.source_cff")
# process.load("CMGTools.Common.sources.HT.Run2011A_HighMET_May10ReReco_v1.RAW_RECO.PAT_CMG_NewJEC.source_cff")
# process.load("CMGTools.Common.sources.HT.Run2011A_HighMET_May10ReReco_v1.RAW_RECO.PAT_CMG_NewJEC_JSON.source_cff")
# process.load("CMGTools.Common.sources.HT.Run2011A_HighMET_PromptSkim_v4.RAW_RECO.PAT_CMG_V2_160404_167284.source_PAT_cff")
# process.load("CMGTools.Common.sources.HT.Run2011A_May10ReReco_v1.AOD.V2.PAT_CMG_TestJune30.source_cff")
# process.load("CMGTools.Common.sources.HT.Run2011A_HighMET_PromptSkim_v4.RAW_RECO.PAT_CMG_V2_0_0_160404_167913.source_cff")
process.load("CMGTools.Common.sources.RelValLM1_sfts.CMSSW_4_2_3_MC_42_V12_v2.GEN_SIM_RECO.PAT_CMG_V2_0_0.source_cff")

nFiles = 10
#print 'WARNING: RESTRICTING INPUT TO THE FIRST', nFiles, 'FILES'
#process.source.fileNames = process.source.fileNames[:nFiles-1] 

# process.source.fileNames = ['file:2011/Loukas/PAT_CMG/tree_CMG.root']

ext = 'CMG'

# output to be stored

print 'processing:'
print process.source.fileNames

outFileNameExt = ext

process.load('CMGTools.Susy.susy_cff')

# process.susySchedule.remove( process.runInfoAccountingPath )

process.schedule = cms.Schedule( process.RA2Path ) 
process.schedule.append( process.outpath )

# pprint.pprint(process.out.outputCommands)
#process.dump = cms.EDAnalyzer("EventContentAnalyzer")

from CMGTools.Susy.susyEventContent_cff import susyEventContent
process.out.fileName = cms.untracked.string('susy_tree_%s.root' %  outFileNameExt)
process.out.outputCommands = cms.untracked.vstring('drop *')
process.out.outputCommands += susyEventContent

# Running only RA2
process.out.SelectEvents = cms.untracked.PSet( SelectEvents = cms.vstring('RA2Path') )

process.TFileService = cms.Service(
    "TFileService",
    fileName = cms.string("susy_histograms_%s.root" %  outFileNameExt)
    )

print process.out.dumpPython()

print 'output file: ', process.out.fileName
# print process.schedule

process.RA2MHTPFJet50CentralSel.cut = 'sumEt()>0'
process.RA2MHTPFJet30Sel.cut = 'et()>0'