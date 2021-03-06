#!/usr/bin/env python
import os,sys
import json
import ROOT
import getopt
import commands

#print usage
def usage() :
    print ' '
    print 'runLocalAnalysisOverSamples.py [options]'
    print '  -s : submit to queue'
    print '  -e : executable name'
    print '  -j : json file containing the samples'
    print '  -d : input dir with the event summaries'
    print '  -o : output directory'
    print '  -c : templated configuration file to steer the job'
    print '  -l : luminosity (pb)'
    print '  -p : extra parameters configure'
    print '  -t : tag to match sample'
    print ' '
    exit(-1)

"""
Gets the value of a given item
(if not available a default value is returned)
"""
def getByLabel(desc,key,defaultVal=None) :
    try :
        return desc[key]
    except KeyError:
        return defaultVal


#parse the options 
try:
     # retrive command line options
     shortopts  = "s:e:j:d:o:c:l:p:t:h?"
     opts, args = getopt.getopt( sys.argv[1:], shortopts )
except getopt.GetoptError:
     # print help information and exit:
     print "ERROR: unknown options in argument %s" % sys.argv[1:]
     usage()
     sys.exit(1)

subtoBatch=False
requirementtoBatch='type==SLC5_64&&pool>30000'
samplesDB=''
theExecutable=''
inputdir=''
outdir=''
lumi=1
cfg_file=''
split=1
segment=0
params=''
onlytag='all'
for o,a in opts:
    if o in("-?", "-h"):
        usage()
        sys.exit(0)
    elif o in('-s'):
        subtoBatch=True
        queue=a
        if(queue=="True") : queue="2nd"
    elif o in('-j'): samplesDB = a
    elif o in('-e'): theExecutable = a
    elif o in('-d'): inputdir = a
    elif o in('-o'): outdir = a
    elif o in('-l'): lumi=float(a)
    elif o in('-c'): cfg_file = a
    elif o in('-p'): params = a
    elif o in('-t'): onlytag = a
                                        
#open the file which describes the sample
jsonFile = open(samplesDB,'r')
procList=json.load(jsonFile,encoding='utf-8').items()

#run over sample
for proc in procList :

    #run over processes
    for desc in proc[1] :

        #run over items in process
        isdata=getByLabel(desc,'isdata',False)
        mctruthmode=getByLabel(desc,'mctruthmode',0)

        data = desc['data']
        for d in data :
            origdtag = getByLabel(d,'dtag','')
            dtag = origdtag
            xsec = getByLabel(d,'xsec',-1)
            br = getByLabel(d,'br',[])
            suffix = str(getByLabel(d,'suffix' ,""))
            if(onlytag!='all') :
                if(dtag.find(onlytag)<0) : continue
            if(mctruthmode!=0) : dtag+='_filt'+str(mctruthmode)
                                
            if(xsec>0 and not isdata) :
                for ibr in br :  xsec = xsec*ibr
            split=getByLabel(d,'split',1)

	    for segment in range(0,split) :
                if(split==1): 
	            	eventsFile=inputdir + '/' + origdtag + '.root'
                else:
                        eventsFile=inputdir + '/' + origdtag + '_' + str(segment) + '.root'

                if(eventsFile.find('/store/')==0)  : eventsFile = commands.getstatusoutput('cmsPfn ' + eventsFile)[1]

            	sedcmd = 'sed \"s%@input%' + eventsFile +'%;s%@outdir%' + outdir +'%;s%@isMC%' + str(not isdata) + '%;s%@mctruthmode%'+str(mctruthmode)+'%;s%@xsec%'+str(xsec)+'%;'
                sedcmd += 's%@cprime%'+str(getByLabel(d,'cprime',-1))+'%;'
                sedcmd += 's%@brnew%' +str(getByLabel(d,'brnew' ,-1))+'%;'
                sedcmd += 's%@suffix%' +suffix+'%;'
            	if(params.find('@useMVA')<0) :          params = '@useMVA=False ' + params
                if(params.find('@weightsFile')<0) :     params = '@weightsFile= ' + params
                if(params.find('@evStart')<0) :         params = '@evStart=0 ' + params
                if(params.find('@evEnd')<0) :           params = '@evEnd=-1 ' + params
            	if(params.find('@saveSummaryTree')<0) : params = '@saveSummaryTree=False ' + params
            	if(params.find('@runSystematics')<0) :  params = '@runSystematics=False ' + params
            	if(len(params)>0) :
                    extracfgs = params.split(' ')
                    for icfg in extracfgs :
                        varopt=icfg.split('=')
                        if(len(varopt)<2) : continue
                        sedcmd += 's%' + varopt[0] + '%' + varopt[1] + '%;'
            	sedcmd += '\"'


		if(split==1): 
	            	cfgfile=outdir +'/'+ dtag + suffix + '_cfg.py'
		else:
                        cfgfile=outdir +'/'+ dtag + suffix + '_' + str(segment) + '_cfg.py'
            	os.system('cat ' + cfg_file + ' | ' + sedcmd + ' > ' + cfgfile)
            	if(not subtoBatch) :
                	os.system(theExecutable + ' ' + cfgfile)
            	else :
			print('submit2batch.sh -q' + queue + ' -R"' + requirementtoBatch + '" -J' + dtag + str(segment) + ' ${CMSSW_BASE}/bin/${SCRAM_ARCH}/wrapLocalAnalysisRun.sh ' + theExecutable + ' ' + cfgfile)
			os.system('submit2batch.sh -q' + queue + ' -R"' + requirementtoBatch + '" -J' + dtag + str(segment) + ' ${CMSSW_BASE}/bin/${SCRAM_ARCH}/wrapLocalAnalysisRun.sh ' + theExecutable + ' ' + cfgfile)
    
#run plotter over results
#if(not subtoBatch) :
#    os.system('mkdir -p ' + outdir + '/plots')
#    #os.system('runPlotterOverSamples.py -j ' + samplesDB + ' -l ' + str(lumi) + ' -i ' + outdir + ' -o ' + outdir + '/plots -d localAnalysis')
#    os.system('runPlotter --json ' + samplesDB + ' --iLumi ' + str(lumi) + ' --inDir ' + outdir + ' --outDir ' + outdir + '/plots')
