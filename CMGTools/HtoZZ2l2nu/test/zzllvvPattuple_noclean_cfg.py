import os,sys
runOnMC=True
runFull=False
runStd=False
cfgFile=os.path.expandvars('${CMSSW_BASE}/src/CMGTools/HtoZZ2l2nu/test/zzllvvPattuple_cfg.py')
from CMGTools.HtoZZ2l2nu.localPatTuples_cff import configureFromCommandLine
castorDir, outFile, inputList = configureFromCommandLine()
execfile(cfgFile)