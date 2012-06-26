import copy
import os 
import CMGTools.RootTools.fwlite.Config as cfg
from CMGTools.H2TauTau.triggerMap import pathsAndFilters


period = 'Period_2011AB'


mc_vertexWeight = None
mc_tauEffWeight = None
mc_muEffWeight = None
mc_tauEffWeight_mc = 'effLooseTau15MC'
mc_muEffWeight_mc = 'effIsoMu15MC'
if period == 'Period_2011A':
    mc_vertexWeight = 'vertexWeightFall112invfb'
    mc_tauEffWeight = 'effTau2011A'
    mc_muEffWeight = 'effMu2011A'
elif period == 'Period_2011B':
    mc_vertexWeight = 'vertexWeightFall112011B'
    mc_tauEffWeight = 'effTau2011B'
    mc_muEffWeight = 'effMu2011B'
elif period == 'Period_2011AB':
    mc_vertexWeight = 'vertexWeightFall112011AB'
    mc_tauEffWeight = 'effTau2011AB'
    mc_muEffWeight = 'effMu2011AB'

triggerAna = cfg.Analyzer(
    'TriggerAnalyzer'
    )

vertexAna = cfg.Analyzer(
    'VertexAnalyzer',
    # goodVertices = 'offlinePrimaryVertices', # hum... collection not available in old tuples
    goodVertices = 'goodPVFilter',
    vertexWeight = mc_vertexWeight,
    verbose = False
    )

TauMuAna = cfg.Analyzer(
    'TauMuAnalyzer',
    pt1 = 20,
    eta1 = 2.3,
    iso1 = 999,
    pt2 = 17,
    eta2 = 2.1,
    iso2 = 0.1,
    m_min = 10,
    m_max = 99999,
    triggerMap = pathsAndFilters
    )

tauWeighter = cfg.Analyzer(
    'LeptonWeighter_tau',
    effWeight = mc_tauEffWeight,
    effWeightMC = mc_tauEffWeight_mc,
    lepton = 'leg1',
    verbose = False
    )

muonWeighter = cfg.Analyzer(
    'LeptonWeighter_mu',
    effWeight = mc_muEffWeight,
    effWeightMC = mc_muEffWeight_mc,
    lepton = 'leg2',
    verbose = False
    )



# defined for vbfAna and eventSorter
vbfKwargs = dict( Mjj = 400,
                  deltaEta = 4.0    
                  )

vbfAna = cfg.Analyzer(
    'VBFAnalyzer',
    vbfMvaWeights = os.environ['CMSSW_BASE'] + '/src/CMGTools/H2TauTau/data/VBFMVA_BDTG.weights.44X.xml',
    jetCol = 'cmgPFJetSel',
    jetPt = 30,
    jetEta = 5.0,
    **vbfKwargs
    )


treeProducer = cfg.Analyzer(
    'H2TauTauTreeProducerTauMu'
    )

#########################################################################################

from CMGTools.H2TauTau.proto.samples.tauMu_ColinJune1 import * 
# from CMGTools.H2TauTau.proto.samples.tauMu_ColinMay18_CHS import * 
# from CMGTools.H2TauTau.proto.samples.tauMu_ColinMay18 import * 
# from CMGTools.H2TauTau.proto.samples.tauMu_ColinMay15 import * 

#########################################################################################

mc_jet_scale = 1.
mc_jet_smear = 0.
for mc in MC:
    # could handle the weights in the same way
    mc.jetScale = mc_jet_scale
    mc.jetSmear = mc_jet_smear


MC = [DYJets, WJets, TTJets]
# MC.extend( mc_higgs )
selectedComponents =  copy.copy(MC)

useEmbed = False 

if period == 'Period_2011A':
    selectedComponents.extend( data_2011A )
    if useEmbed:
        selectedComponents.extend( embed_2011A )    
elif period == 'Period_2011B':
    selectedComponents.extend( data_2011B )
    if useEmbed:
        selectedComponents.extend( embed_2011B )    
elif period == 'Period_2011AB':
    selectedComponents.extend( data_2011 )
    if useEmbed:
        selectedComponents.extend( embed_2011 )    
    



sequence = cfg.Sequence( [
    triggerAna,
    vertexAna,
    TauMuAna,
    vbfAna,
    tauWeighter, 
    muonWeighter, 
    treeProducer
   ] )


DYJets.fakes = True
DYJets.splitFactor = 40
WJets.splitFactor = 10
TTJets.splitFactor = 100

data_Run2011B_PromptReco_v1.splitFactor = 50
data_Run2011A_PromptReco_v4.splitFactor = 40
data_Run2011A_May10ReReco_v1.splitFactor = 40
data_Run2011A_05Aug2011_v1.splitFactor = 20
data_Run2011A_03Oct2011_v1.splitFactor = 20
    
embed_Run2011B_PromptReco_v1.splitFactor = 10
embed_Run2011A_PromptReco_v4.splitFactor = 10
embed_Run2011A_May10ReReco_v1.splitFactor = 5
embed_Run2011A_05Aug2011_v1.splitFactor = 5
embed_Run2011A_03Oct2011_v1.splitFactor = 5

test = 0
if test==1:
    comp = DYJets
    comp.files = comp.files[:2]
    # comp = data_2012[0]
    selectedComponents = [comp]
    comp.splitFactor = 1
elif test==2:
    for comp in selectedComponents:
        comp.splitFactor = 1
        # comp.files = comp.files[:2]

# selectedComponents.extend(MC)

config = cfg.Config( components = selectedComponents,
                     sequence = sequence )