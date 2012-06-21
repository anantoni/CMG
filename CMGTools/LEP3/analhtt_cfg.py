import copy
import os
import CMGTools.RootTools.fwlite.Config as cfg

#fourJetAna = cfg.Analyzer(
#    'FourJetAnalyzer',
#    npfj = (3,5),       # at least 8 PF particle in 3 of the 4 jets
#    ntkj = (4,1),       # at least 1 PF charged hadron in 4 of the 4 jets
#    minM = 0.5,         # No jet with a jet mass smaller than 2.5 GeV
#    mVis = 180.,        # total visible mass in excess of 180 GeV,
#    chi2 = 1000.,       # total chi**2 of the beta4 "fit" smaller than 1000. (i.e., all fit energies > 0.)    
#    pair_mass = 0.,    # No jet pair should have a mass smaller than 30 GeV
#    pair_cijkl = 1000.,  # Tue smallest sum of cos(ij)+cos(kl) should be larger than -1.1
#    pair_sumtet = 0., # The sum of the four smallest jet-jet angles must be larger than 350 degrees
#    h_mass = 105.,       # The Higgs candidate mass should be larger than 90 GeV
#    z_mass = (80.,110.),     # The Z candidate jet pair should have a mass closer to the Z mass than 100 GeV
#    )

httAna = cfg.Analyzer('httanalyzer',
  npfj = (2,3),       # at least 8 PF particle in 3 of the 4 jets
  ntkj = (2,1),
  minM = 0.5,
  mVis = 180.,
  chi2 = 1000.,
  minE = 10.,
  h_mass = 105.,
  z_mass = (80.,110.),
)

def createTreeProducer( ana ):
    tp = cfg.Analyzer( '_'.join( ['htttreeproducer','httanalyzer'] ),
                       anaName = 'httanalyzer'
                       )
    return tp


# for debugging 
stopper = cfg.Analyzer(
    'Stopper'
    )

sequence = cfg.Sequence([
    httAna,
    createTreeProducer( httAna ),
    ])


Hig125 = cfg.MCComponent(
    name = 'Hig125',
    zzfiles = ['root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_80.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_81.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_82.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_83.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_84.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_85.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_86.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_87.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_88.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_89.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_90.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_91.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_92.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_93.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_94.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_95.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_96.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_97.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_98.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/ZZ/cmgTuple_ZZ_99.root',],
    files = ['root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_0.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_1.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_2.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_3.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_4.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_5.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_6.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_7.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_8.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_9.root',],
    tufiles = ['root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_170.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_171.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_172.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_173.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_174.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_175.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_176.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_177.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_178.root',
             #'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_179.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_180.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_181.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_182.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_183.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_184.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_185.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_186.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_187.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_188.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_189.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_190.root',
             #'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_191.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_192.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_193.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_194.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_195.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_196.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_197.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_198.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_199.root',
             #'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_93.root',
             #'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_179.root',
             #'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_191.root',
             ],
    tfiles = ['root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_270.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_271.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_272.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_273.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_274.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_275.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_276.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_277.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_278.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_279.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_280.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_281.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_282.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_283.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_284.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_285.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_286.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_287.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_288.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_289.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_290.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_291.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_292.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_293.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_294.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_295.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_296.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_297.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_298.root',
             'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_299.root',
             #'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_21.root',
             #'root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_33.root',
             ],
    yfiles = ['root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/cmgTuple_HZHA_0.root',],
    zfiles = ['root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/WW/cmgTuple_WW_0.root',],
    wfiles = ['root://eoscms//eos/cms/store/cmst3/user/pjanot/LEP3/QQBAR/cmgTuple_QQBAR_0.root',],
    xSection = 0.003213106 * 0.863920605, 
    nGenEvents = 1, # dummy 
    triggers = [],
    intLumi = 1000,
    effCorrFactor = 1 )

selectedComponents = [Hig125]

Hig125.splitFactor = 1
    
config = cfg.Config( components = selectedComponents,
                     sequence = sequence )
