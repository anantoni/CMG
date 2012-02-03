import os 
import FWCore.ParameterSet.Config as cms


def addOutput( process, type12, debugEventContent=False):

    allowedTypes = ['tauMu', 'tauEle', 'muEle']
    if type12 not in allowedTypes:
        raise ValueError( type12 + ' not in allowed types: ', allowedTypes )

    # skim (basic selection)     ------
    outFileNameExt = 'CMG'
    basicName = '{type}_presel_tree_{ext}.root'.format(
        type = type12,
        ext = outFileNameExt
        )
    
    # process.out.fileName = cms.untracked.string( basicName )
    from CMGTools.H2TauTau.eventContent.tauMu_cff import tauMu as tauMuEventContent
    from CMGTools.H2TauTau.eventContent.tauMu_cff import tauMuDebug as tauMuDebugEventContent
    from CMGTools.H2TauTau.eventContent.tauEle_cff import tauEle as tauEleEventContent
    from CMGTools.H2TauTau.eventContent.tauEle_cff import tauEleDebug as tauEleDebugEventContent
    from CMGTools.H2TauTau.eventContent.muEle_cff import muEle as muEleEventContent
    from CMGTools.H2TauTau.eventContent.muEle_cff import muEleDebug as muEleDebugEventContent

    eventContent = None
    debugEventContent = None
    if type12=='tauMu':
        eventContent = tauMuEventContent
        debugEventContent = tauMuDebugEventContent
    elif type12=='tauEle':
        eventContent = tauEleEventContent
        debugEventContent = tauEleDebugEventContent
    elif type12=='muEle':
        eventContent = muEleEventContent
        debugEventContent = muEleDebugEventContent
        
    
    out = cms.OutputModule(
        "PoolOutputModule",
        fileName = cms.untracked.string( basicName ),
        # save only events passing the full path
        SelectEvents   = cms.untracked.PSet(
           SelectEvents = cms.vstring('{type12}PreSelPath'.format(type12=type12))
        ),
        # save PAT Layer 1 output; you need a '*' to
        # unpack the list of commands 'patEventContent'
        outputCommands = cms.untracked.vstring('drop *')
        )
    if debugEventContent:
        out.outputCommands.extend( debugEventContent )
    else:
        out.outputCommands.extend( eventContent )


    # full baseline selection    ------

    outBaseline = out.clone()
    outBaseline.SelectEvents = cms.untracked.PSet(
        SelectEvents = cms.vstring('{type12}FullSelPath'.format(type12=type12))
        )
    baselineName = '{type12}_fullsel_tree_{ext}.root'.format(
        type12 = type12,
        ext = outFileNameExt
        )
    outBaseline.fileName = baselineName

    setattr( process, os.path.splitext(basicName)[0], out )
    setattr( process, os.path.splitext(baselineName)[0], outBaseline )
    
    process.outpath += out
    process.outpath += outBaseline
    print 'adding output:', outBaseline.fileName
    print 'adding output:', out.fileName
    
    

def addTauMuOutput( process, debugEventContent=False ):
    addOutput(process,'tauMu', debugEventContent)

def addTauEleOutput( process, debugEventContent=False ):
    addOutput(process,'tauEle', debugEventContent)
 
def addMuEleOutput( process, debugEventContent=False ):
    addOutput(process,'muEle', debugEventContent)
 
