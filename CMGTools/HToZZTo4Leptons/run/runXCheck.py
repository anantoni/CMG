
import copy
import os 
import CMGTools.RootTools.fwlite.Config as cfg

from CMGTools.HToZZTo4Leptons.setup.EffectiveAreas import effectiveAreas2012 as effectiveAreas
from CMGTools.HToZZTo4Leptons.setup.FSR import FSRConfig as fsr

channel = 'all'

skimAnalyzer = cfg.Analyzer('SkimCountAnalyzer')

A4Skim = cfg.Analyzer('A4Skim',
    muons='cmgMuonSel',
    electrons='cmgElectronSel'
    )


#Trigger Information
triggerAna = cfg.Analyzer(
    'TriggerAnalyzer'
    )

#Vertex information
vertexAna = cfg.Analyzer(
    'VertexAnalyzer',
    fixedWeight = 1,
    verbose = False,
    makeHists=False
    )


#Pu reweighting
puAna = cfg.Analyzer(
    "PileUpAnalyzer",
    # build unweighted pu distribution using number of pile up interactions if False
    # otherwise, use fill the distribution using number of true interactions
    true = True,
    makeHists=False
    )


#Analyzers
muMuAna = cfg.Analyzer(
    'MuMuFourLeptonAnalyzer',
    minPt1=5,
    maxEta1=2.4,
    minPt2=5,
    maxEta2=2.4,
    z1_m = (40.,120.),
    z2_m = (4.,120.),
    z1_pt1 = 20,
    z1_pt2 = 10,
    minHMass=100.,
    minHMassZ2=12.,
    minMass=4.,
    keep=False,
    effectiveAreas=effectiveAreas,
    rhoMuon     = 'kt6PFJetsCentralNeutral',
    rhoElectron = 'kt6PFJets',
    FSR=fsr
    )


muEleAna = copy.deepcopy( muMuAna)
muEleAna.name = 'MuEleFourLeptonAnalyzer'
muEleAna.minPt1 = 5
muEleAna.maxEta1 = 2.4
muEleAna.minPt2 = 7
muEleAna.maxEta2 = 2.5


eleEleAna = copy.deepcopy( muEleAna )
eleEleAna.name = 'EleEleFourLeptonAnalyzer'
eleEleAna.minPt1 = 7
eleEleAna.maxEta1 = 2.5
eleEleAna.minPt2 = 7
eleEleAna.maxEta2 = 2.5


allAna = copy.deepcopy( muMuAna)
allAna.name = 'AllFourLeptonAnalyzer'
allAna.minPt1 = 5
allAna.maxEta1 = 2.4
allAna.minPt2 = 7
allAna.maxEta2 = 2.5


zzTree = cfg.Analyzer( 'FourLeptonTreeProducer',
                   effectiveAreas=effectiveAreas
)


####################################################################################

from CMGTools.HToZZTo4Leptons.samples.samples_8TeV import * 

####################################################################################


jsonFilter = cfg.Analyzer(
    'JSONAnalyzer'
    )



    
theAna = None
if channel == 'mu_mu':
    theAna = muMuAna
    for data in dataSamplesMu:
        data.triggers = triggers_mumu
    for mc in mcSamples:
        mc.triggers = triggersMC_mumu
    selectedComponents=mcSamples+dataSamplesMu
    
elif channel == 'mu_ele' :
    theAna = muEleAna
    for data in dataSamplesE:
        data.triggers = triggers_ee
    for data in dataSamplesMu:
        data.triggers = triggers_mumu
        data.vetoTriggers = triggers_ee

    for data in dataSamplesMuE:
        data.triggers = triggers_mue
        data.vetoTriggers=triggers_ee+triggers_mumu

    for mc in mcSamples:
        mc.triggers = triggersMC_mue
    selectedComponents=mcSamples+dataSamplesMu+dataSamplesE+dataSamplesMuE

elif channel == 'all' :
    theAna = allAna
    for data in dataSamplesE:
        data.triggers = triggers_ee
    for data in dataSamplesMu:
        data.triggers = triggers_mumu
        data.vetoTriggers = triggers_ee

    for data in dataSamplesMuE:
        data.triggers = triggers_mue
        data.vetoTriggers=triggers_ee+triggers_mumu

    for mc in mcSamples:
        mc.triggers = triggersMC_mue
    selectedComponents=mcSamples+dataSamplesMu+dataSamplesE+dataSamplesMuE
#    selectedComponents=dataSamplesMu

    

elif channel == 'ele_ele':
    theAna = eleEleAna
    for data in dataSamplesE:
        data.triggers = triggers_ee
    for mc in mcSamples:
        mc.triggers = triggersMC_ee
    selectedComponents=mcSamples+dataSamplesE

#Define Sequences for data and MC

EventSelector = cfg.Analyzer(
    'EventSelector',
    toSelect = [66761]
    )


dataSequence=[
#    EventSelector,
    skimAnalyzer,
    jsonFilter,
    puAna,
#    A4Skim,
    triggerAna,
    vertexAna,
    theAna,
    zzTree
    ]



#for full production use data sequence
sequence = cfg.Sequence(dataSequence)





test = 1
if test==1:
    dataset = GGH127
    selectedComponents = [dataset]
    dataset.splitFactor = 1
    dataset.files=['ggh.root']
#    dataset.files=['cmgTupleVBF.root']


   
if test ==2:
    selectedComponents=selectedComponents[:1]
    print selectedComponents
    for comp in selectedComponents:
        comp.splitFactor = 1
        comp.files = comp.files[:10]

if test ==3:
    selectedComponents=selectedComponents[-1:]
    print selectedComponents
    for comp in selectedComponents:
        comp.splitFactor = 1
        comp.files = comp.files[:10]



    
config = cfg.Config( components = selectedComponents,
                     sequence = sequence )
