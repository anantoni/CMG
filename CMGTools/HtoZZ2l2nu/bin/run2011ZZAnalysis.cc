#include <iostream>
#include <boost/shared_ptr.hpp>

#include "EGamma/EGammaAnalysisTools/interface/EGammaCutBasedEleId.h"

#include "CMGTools/HtoZZ2l2nu/interface/ZZ2l2nuSummaryHandler.h"
#include "CMGTools/HtoZZ2l2nu/interface/ZZ2l2nuPhysicsEvent.h"
#include "CMGTools/HtoZZ2l2nu/interface/METUtils.h"
#include "CMGTools/HtoZZ2l2nu/interface/GammaEventHandler.h"
#include "CMGTools/HtoZZ2l2nu/interface/setStyle.h"
#include "CMGTools/HtoZZ2l2nu/interface/plotter.h"
#include "CMGTools/HtoZZ2l2nu/interface/ObjectFilters.h"
#include "CMGTools/HtoZZ2l2nu/interface/SmartSelectionMonitor.h"
#include "CMGTools/HtoZZ2l2nu/interface/TMVAUtils.h"
#include "CMGTools/HtoZZ2l2nu/interface/MacroUtils.h"
#include "CMGTools/HtoZZ2l2nu/interface/EventCategory.h"
#include "CMGTools/HtoZZ2l2nu/interface/EfficiencyMap.h"

#include "CondFormats/JetMETObjects/interface/JetResolution.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"

#include "FWCore/FWLite/interface/AutoLibraryLoader.h"
#include "FWCore/PythonParameterSet/interface/MakeParameterSets.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "PhysicsTools/Utilities/interface/LumiReWeighting.h"

#include "TSystem.h"
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include "TEventList.h"
#include "TROOT.h"
 
using namespace std;

TString getJetRegion(float eta)
{
  TString reg("TK");
  if(fabs(eta)>2.5)  reg="HEin";
  if(fabs(eta)>2.75) reg="HEout";
  if(fabs(eta)>3)    reg="HF";
  return reg;
}


int main(int argc, char* argv[])
{
  //##############################################
  //########    GLOBAL INITIALIZATION     ########
  //##############################################

  // check arguments
  if(argc<2){ std::cout << "Usage : " << argv[0] << " parameters_cfg.py" << std::endl; exit(0); }
  
  // load framework libraries
  gSystem->Load( "libFWCoreFWLite" );
  AutoLibraryLoader::enable();
  
  // configure the process
  const edm::ParameterSet &runProcess = edm::readPSetsFrom(argv[1])->getParameter<edm::ParameterSet>("runProcess");

  bool use2011Id = runProcess.getParameter<bool>("is2011");
  cout << "Note: will apply " << (use2011Id ? 2011 : 2012) << " version of the id's" << endl;

  bool isMC       = runProcess.getParameter<bool>("isMC");
  bool runBlinded = runProcess.getParameter<bool>("runBlinded"); 
  int mctruthmode = runProcess.getParameter<int>("mctruthmode");

  TString url=runProcess.getParameter<std::string>("input");
  TString outFileUrl(gSystem->BaseName(url)); 
  outFileUrl.ReplaceAll(".root",""); 
  if(mctruthmode!=0) { outFileUrl += "_filt"; outFileUrl += mctruthmode; }
  TString outdir=runProcess.getParameter<std::string>("outdir");
  TString outUrl( outdir );
  gSystem->Exec("mkdir -p " + outUrl);
  int fType(0);
  if(url.Contains("DoubleEle")) fType=EE;
  if(url.Contains("DoubleMu"))  fType=MUMU;
  if(url.Contains("MuEG"))      fType=EMU;
  if(url.Contains("SingleMu"))  fType=MUMU;
  bool isSingleMuPD(!isMC && url.Contains("SingleMu"));  
  
  TString outTxtUrl= outUrl + "/" + outFileUrl + ".txt";
  FILE* outTxtFile = NULL;
  //if(!isMC)
  outTxtFile = fopen(outTxtUrl.Data(), "w");
  printf("TextFile URL = %s\n",outTxtUrl.Data());

  //tree info
  int evStart     = runProcess.getParameter<int>("evStart");
  int evEnd       = runProcess.getParameter<int>("evEnd");
  TString dirname = runProcess.getParameter<std::string>("dirName");

  //jet energy scale uncertainties
  TString uncFile = runProcess.getParameter<std::string>("jesUncFileName"); gSystem->ExpandPathName(uncFile);
  JetCorrectionUncertainty jecUnc(uncFile.Data());

  //systematics
  bool runSystematics                        = runProcess.getParameter<bool>("runSystematics");
  TString varNames[]={"",
		      "_jerup","_jerdown",
		      "_jesup","_jesdown",
		      "_umetup","_umetdown",
		      "_lesup","_lesdown",
		      "_puup","_pudown",
		      "_renup","_rendown",
		      "_factup","_factdown",
		      "_btagup","_btagdown",
		      "_lshapeup","_lshapedown",
 		      "_sherpaup","_sherpadown"};
  size_t nvarsToInclude(1);
  if(runSystematics)
    {
      cout << "Systematics will be computed for this analysis" << endl;
      nvarsToInclude=sizeof(varNames)/sizeof(TString);
    }

  // this is disabled for the moment
  double HiggsMass=0; string VBFString = ""; string GGString("");
  bool isMC_GG  = isMC && ( string(url.Data()).find("GG" )  != string::npos);
  bool isMC_VBF = isMC && ( string(url.Data()).find("VBF")  != string::npos);
  TFile *fin;
  int cmEnergy(8);
  if(url.Contains("7TeV")) cmEnergy=7;
  std::vector<TGraph *> hWeightsGrVec,hLineShapeGrVec;
  if(isMC_GG){  
    size_t GGStringpos =  string(url.Data()).find("GG");
    string StringMass = string(url.Data()).substr(GGStringpos+5,4);  sscanf(StringMass.c_str(),"%lf",&HiggsMass);
    GGString = string(url.Data()).substr(GGStringpos);
  
    //H pT
    if(cmEnergy==7){
      TString hqtWeightsFileURL = runProcess.getParameter<std::vector<std::string> >("hqtWeightsFile")[0]; gSystem->ExpandPathName(hqtWeightsFileURL);
      fin=TFile::Open(hqtWeightsFileURL);
      if(fin){
	cout << "HpT weights (and uncertainties) will be applied from: " << hqtWeightsFileURL << endl;
	for(int ivar=0; ivar<5; ivar++){
	  double ren=HiggsMass; if(ivar==ZZ2l2nuSummary_t::hKfactor_renDown)  ren/=2; if(ivar==ZZ2l2nuSummary_t::hKfactor_renUp)  ren *= 2;
	  double fac=HiggsMass; if(ivar==ZZ2l2nuSummary_t::hKfactor_factDown) fac/=2; if(ivar==ZZ2l2nuSummary_t::hKfactor_factUp) fac *= 2;
	  char buf[100]; sprintf(buf,"kfactors_mh%3.0f_ren%3.0f_fac%3.0f",HiggsMass,ren,fac);
	  TGraph *gr= (TGraph *) fin->Get(buf);
	  if(gr) hWeightsGrVec.push_back((TGraph *)gr->Clone());
	}
	fin->Close();
	delete fin;
      }
    }    
  }else if(isMC_VBF){
    size_t VBFStringpos =  string(url.Data()).find("VBF");
    string StringMass = string(url.Data()).substr(VBFStringpos+6,4);  sscanf(StringMass.c_str(),"%lf",&HiggsMass);
    VBFString = string(url.Data()).substr(VBFStringpos);
  }
  
  //LINE SHAPE WEIGHTS
  TString lineShapeWeightsFileURL = runProcess.getParameter<std::vector<std::string> >("hqtWeightsFile")[1]; gSystem->ExpandPathName(lineShapeWeightsFileURL);
  fin=0;
  std::vector<TString> wgts;
  if(isMC_VBF)
    {
      char buf[100]; sprintf(buf,"H%d/",int(HiggsMass));
      wgts.push_back(buf+TString("cpsWgt"));
      wgts.push_back(buf+TString("cpsUpWgt"));
      wgts.push_back(buf+TString("cpsDownWgt"));
      wgts.push_back(buf+TString("cpsWgt"));
      lineShapeWeightsFileURL.ReplaceAll("LineShapeWeights","VBFLineShapeWeights");
      fin=TFile::Open(lineShapeWeightsFileURL);     
    }
  else
    {
      char buf[100]; sprintf(buf,"Higgs%d_%dTeV/",int(HiggsMass),cmEnergy);
      wgts.push_back(buf+TString("rwgtpint"));
      wgts.push_back(buf+TString("rwgtpint_up"));
      wgts.push_back(buf+TString("rwgtpint_down"));
      wgts.push_back(buf+TString("rwgt"));
      fin=TFile::Open(lineShapeWeightsFileURL);     
    }
  if(fin)
    {
      cout << "Line shape weights (and uncertainties) will be applied from " << fin->GetName() << endl;
      for(size_t i=0; i<wgts.size(); i++)
	{
	  TGraph *gr= (TGraph *) fin->Get(wgts[i]);
	  if(gr) hLineShapeGrVec.push_back((TGraph *)gr->Clone());
       	}
      fin->Close();
      delete fin;
    }


  //##############################################
  //########    INITIATING HISTOGRAMS     ########
  //##############################################
  SmartSelectionMonitor mon;

  mon.addHistogram(  new TProfile("metvsrun"    ,      "Run number",     600, 190000,196000) ) ;
  mon.addHistogram(  new TProfile("metvsavginstlumi",  "Avg. inst lumi", 60,  400,1000));
  mon.addHistogram(  new TProfile("nvtxvsrun",         "Run number",     600, 190000,196000) ) ;
  mon.addHistogram(  new TProfile("nvtxvsavginstlumi", "Avg. inst lumi", 60,  400,1000));

  TH1F* Hcutflow  = (TH1F*) mon.addHistogram(  new TH1F ("cutflow"    , "cutflow"    ,6,0,6) ) ;
  TH1F *h=(TH1F*) mon.addHistogram( new TH1F ("eventflow", ";;Events", 11,0,11) );
  h->GetXaxis()->SetBinLabel(1,"Trigger");
  h->GetXaxis()->SetBinLabel(2,"#geq 2 leptons");
  h->GetXaxis()->SetBinLabel(3,"#geq 2 iso leptons");
  h->GetXaxis()->SetBinLabel(4,"|M-M_{Z}|<15");
  h->GetXaxis()->SetBinLabel(5,"p_{T}^{ll}>55");
  h->GetXaxis()->SetBinLabel(6,"3^{rd}-lepton veto");
  h->GetXaxis()->SetBinLabel(7,"Jet veto"); 
  h->GetXaxis()->SetBinLabel(8,"b-veto"); 
  h->GetXaxis()->SetBinLabel(9,"#Delta #phi(jet,E_{T}^{miss})>0.5");
  h->GetXaxis()->SetBinLabel(10,"E_{T}^{miss}>70");
  h->GetXaxis()->SetBinLabel(11,"0.4<E_{T}^{miss}/p_{T}^{ll}<1.8");

  h=(TH1F*) mon.addHistogram( new TH1F ("eventflow_MVA", ";;Events", 11,0,11) );
  h->GetXaxis()->SetBinLabel(1,"Trigger");
  h->GetXaxis()->SetBinLabel(2,"#geq 2 leptons");
  h->GetXaxis()->SetBinLabel(3,"#geq 2 iso leptons");
  h->GetXaxis()->SetBinLabel(4,"|M-M_{Z}|<15");
  h->GetXaxis()->SetBinLabel(5,"p_{T}^{ll}>55");
  h->GetXaxis()->SetBinLabel(6,"3^{rd}-lepton veto");
  h->GetXaxis()->SetBinLabel(7,"Jet veto"); 
  h->GetXaxis()->SetBinLabel(8,"b-veto"); 
  h->GetXaxis()->SetBinLabel(9,"#Delta #phi(jet,E_{T}^{miss})>0.5");
  h->GetXaxis()->SetBinLabel(10,"E_{T}^{miss}>70");
  h->GetXaxis()->SetBinLabel(11,"0.4<E_{T}^{miss}/p_{T}^{ll}<1.8");

  mon.addHistogram( new TH1F ("syncheventflow", ";;Events", 9,0,9) );

  mon.addHistogram( new TH1F( "leadpt", ";p_{T}^{l};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "leadeta", ";#eta^{l};Events", 50,-2.6,2.6) );
  mon.addHistogram( new TH1F( "trailerpt", ";p_{T}^{l};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "trailereta", ";#eta^{l};Events", 50,-2.6,2.6) );
  //  mon.addHistogram( new TH1F( "deltaleptonpt", ";|p_{T}^{1}-p_{T}^{2}|;Events", 50,0,250) );
  // mon.addHistogram( new TH1F( "deltazpt", ";p_{T}^{ll}-E_{T}^{miss};Events", 50,-250,250) );
  mon.addHistogram( new TH1F( "zpt", ";p_{T}^{ll};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "zpt_noJet", ";p_{T}^{ll};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "zptGen_noJet", ";p_{T}^{ll};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "zptNM1", ";p_{T}^{ll};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "zeta", ";#eta^{ll};Events", 50,-10,10) );
  mon.addHistogram( new TH1F( "zmass", ";M^{ll};Events", 100,40,250) );
  mon.addHistogram( new TH1F( "zmassNM1", ";M^{ll};Events", 100,40,250) );

//   ((TH1F*)mon.addHistogram( new TH1F( "higgsMass_0raw", ";Gen Higgs Mass;Events", 500,0,1500) ))->Fill(-1.0,0.0001);//add an underflow entry to make sure the histo is kept
//   ((TH1F*)mon.addHistogram( new TH1F( "higgsMass_1vbf", ";Gen Higgs Mass;Events", 500,0,1500) ))->Fill(-1.0,0.0001);//add an underflow entry to make sure the histo is kept
//   ((TH1F*)mon.addHistogram( new TH1F( "higgsMass_2qt" , ";Gen Higgs Mass;Events", 500,0,1500) ))->Fill(-1.0,0.0001);//add an underflow entry to make sure the histo is kept
//   ((TH1F*)mon.addHistogram( new TH1F( "higgsMass_sls" , ";Gen Higgs Mass;Events", 1000,0,2000) ))->Fill(-1.0,0.0001);//add an underflow entry to make sure the histo is kept
//   ((TH1F*)mon.addHistogram( new TH1F( "higgsMass_3ls" , ";Gen Higgs Mass;Events", 500,0,1500) ))->Fill(-1.0,0.0001);//add an underflow entry to make sure the histo is kept
//   ((TH1F*)mon.addHistogram( new TH1F( "higgsMass_ls" ,  ";Gen Higgs Mass;Events", 500,0,1500) ))->Fill(-1.0,0.0001);//add an underflow entry to make sure the histo is kept

  // 3rd lepton control 
  mon.addHistogram( new TH1F( "thirdleptonpt", ";p_{T}^{l};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "thirdleptoneta", ";#eta^{l};Events", 50,-2.6,2.6) );
  mon.addHistogram( new TH1F( "nleptons", ";Leptons;Events", 3,2,4) );
  mon.addHistogram( new TH1F( "nleptons_final", ";Leptons;Events", 3,2,4) );
  mon.addHistogram( new TH1F( "nleptonsNM1", ";Leptons;Events", 3,2,4) );
  // mon.addHistogram( new TH1F( "ngenjets", ";Gen. jets;Events", 4,0,4) );
  mon.addHistogram( new TH1F( "mt3" , ";M_{T}^{3rd lepton} [GeV/c^{2}];Events",20,0,200) );

//   h=(TH1F*) mon.addHistogram( new TH1F ("vbfeventflow", ";;Events", 8,0,8) );
//   h->GetXaxis()->SetBinLabel(1,"2 gen. leptons");
//   h->GetXaxis()->SetBinLabel(2,"#geq 2 leptons");
//   h->GetXaxis()->SetBinLabel(3,"#geq 2 jets");
//   h->GetXaxis()->SetBinLabel(4,"#Delta #eta(j,j)");
//   h->GetXaxis()->SetBinLabel(5,"M(j,j)");
//   h->GetXaxis()->SetBinLabel(6,"CJV");
//   h->GetXaxis()->SetBinLabel(7,"#Delta #phi(jet,E_{T}^{miss})>0.5");
//   h->GetXaxis()->SetBinLabel(8,"E_{T}^{miss}>70");
//   h=(TH1F*) mon.addHistogram( new TH1F ("vbfmode", ";;Events", 3,0,3) );
//   h->GetXaxis()->SetBinLabel(1,"ll");
//   h->GetXaxis()->SetBinLabel(2,"l#tau");
//   h->GetXaxis()->SetBinLabel(3,"#tau#tau");
//   mon.addHistogram( new TH1F( "genmjj", ";Gen. M_{j,j};Events", 50,0,2500) );
//   mon.addHistogram( new TH1F( "gendeta", ";Gen. #Delta #eta;Events", 50,0,10) );
//   mon.addHistogram( new TH1F( "genjet1pt", ";Gen. jet #1;Events", 50,0,250) );
//   mon.addHistogram( new TH1F( "genjet2pt", ";Gen. jet #2;Events", 50,0,250) );


  //LEPTON control

  //RunDependentVariable
  mon.addHistogram( new TH1F( "RunDep_Yields", ";Run;Events",4000,170000,210000) );
  mon.addHistogram( new TProfile( "RunDep_Met", ";Run;<Met>",4000,170000,210000) );

  //lepton control
  //  Double_t effptAxis[]={0,10,20,30,40,50,75,100,150,200,500};
  // const size_t nEffptAxis=sizeof(effptAxis)/sizeof(Double_t)-1;
  for(size_t ilep=0; ilep<2; ilep++)
    {
      TString lepStr(ilep==0? "mu" :"e");
      //       mon.addHistogram(new TH1F(lepStr+"genpt",   ";p_{T} [GeV/c];Leptons",nEffptAxis,effptAxis) );
      //       mon.addHistogram(new TH1F(lepStr+"geneta",   ";#eta;Leptons",50,-5,5) );
      //       mon.addHistogram(new TH1F(lepStr+"genpu",   ";Pileup;Leptons",50,0,50) );
      //       for(int iid=0; iid<4; iid++)
      // 	{
      // 	  TString idctr(""); idctr+=iid;
      // 	  mon.addHistogram(new TH1F(lepStr+idctr+"pt",   ";p_{T} [GeV/c];Leptons",nEffptAxis,effptAxis) );
      // 	  mon.addHistogram(new TH1F(lepStr+idctr+"eta",   ";#eta;Leptons",50,-5,5) );
      // 	  mon.addHistogram(new TH1F(lepStr+idctr+"pu",   ";Pileup;Leptons",50,0,50) );
      // 	  mon.addHistogram(new TH1F(lepStr+idctr+"isopt",   ";p_{T} [GeV/c];Leptons",nEffptAxis,effptAxis) );
      // 	  mon.addHistogram(new TH1F(lepStr+idctr+"isoeta",   ";#eta;Leptons",50,-5,5) );
      // 	  mon.addHistogram(new TH1F(lepStr+idctr+"isopu",   ";Pileup;Leptons",50,0,50) );
      // 	}
      //       if(ilep==1)
      // 	{
      // 	  for(size_t ireg=0; ireg<2; ireg++)
      // 	    { 
      // 	      TString reg(ireg==0?"eb":"ee");
      // 	      mon.addHistogram(new TH1F(lepStr+reg+"detain",   ";#Delta#eta_{in};Leptons",50,0,0.01) );
      // 	      mon.addHistogram(new TH1F(lepStr+reg+"dphiin",   ";#Delta#phi_{in};Leptons",50,0,0.1) );
      // 	      mon.addHistogram(new TH1F(lepStr+reg+"sihih",    ";#sigma_{i#eta i#eta};Leptons",50,0,0.05) );
      // 	      mon.addHistogram(new TH1F(lepStr+reg+"sipip",    ";#sigma_{i#phi i#phi};Leptons",50,0,0.05) );
      // 	      mon.addHistogram(new TH1F(lepStr+reg+"r9",       ";R_{9};Leptons",50,0,1.) );
      // 	      mon.addHistogram(new TH1F(lepStr+reg+"hoe",      ";h/e;Leptons",50,0,0.2) );
      // 	      mon.addHistogram(new TH1F(lepStr+reg+"ooemoop",  ";1/E-1/p;Leptons",100,0,0.05) );
      // 	      mon.addHistogram(new TH1F(lepStr+reg+"eopin",    ";E/p;Leptons",100,0,2) );
      // 	      mon.addHistogram(new TH1F(lepStr+reg+"fbrem",    ";f_{brem};Leptons",100,0,2) );
      // 	    }
      // 	}
      //       else
      // 	{
      // 	  mon.addHistogram(new TH1F(lepStr+"nmatches", ";Muon matches;Leptons",15,0,15) );
      // 	  mon.addHistogram(new TH1F(lepStr+"nmuonhits", ";Muon hits;Leptons",30,0,30) );
      // 	}
      //       mon.addHistogram(new TH1F(lepStr+"d0",            ";d_{0};Leptons",50,0,0.05) );
      //       mon.addHistogram(new TH1F(lepStr+"dZ",            ";d_{Z};Leptons",50,0,0.1) );
      //       mon.addHistogram(new TH1F(lepStr+"trkchi2",       ";#chi^{2};Leptons",50,0,10) );
      //       mon.addHistogram(new TH1F(lepStr+"trkvalidpixel",  ";Valid pixel hits;Leptons",20,0,20) );
      //       mon.addHistogram(new TH1F(lepStr+"trkvalidtracker",  ";Valid tracker hits;Leptons",50,0,50) );
      //       mon.addHistogram(new TH1F(lepStr+"losthits",         ";Lost hits;Leptons",4,0,4) );
      mon.addHistogram(new TH1F(lepStr+"reliso",           ";RelIso;Leptons",50,0,2) );
      //       mon.addHistogram(new TH1F(lepStr+"truereliso",           ";RelIso;Leptons",50,0,2) );
      //       mon.addHistogram(new TH1F(lepStr+"fakereliso",           ";RelIso;Fake leptons",50,0,2) );
    }
  
  // pileup control
  mon.addHistogram( new TH1F( "nvtx",";Vertices;Events",50,0,50) ); 
  mon.addHistogram( new TH1F( "nvtxraw",";Vertices;Events",50,0,50) ); 
  mon.addHistogram( new TH1F( "rho",";#rho;Events",50,0,25) ); 
  mon.addHistogram( new TH1F( "rho25",";#rho(#eta<2.5);Events",50,0,25) ); 

  // background control
  mon.addHistogram( new TH1F( "Ctrl_WZ_RedMet",";RedMet [GeV];Events",100,0,300) );
  mon.addHistogram( new TH1F( "Ctrl_WZ_zmass","Mass [GeV];Ctrl_WZ;Events",100,40,170) );
  mon.addHistogram( new TH1F( "Ctrl_WZ_Mt","Trasv. Mass [GeV];Ctrl_WZ;Events",100,0.,130) );
  mon.addHistogram( new TH1F( "Ctrl_T_zmass",";Mass [GeV];Events",50,40,300) );
  mon.addHistogram( new TH1F( "Ctrl_Tside_RedMet",";RedMet [GeV];Events",50,0,300) );
  mon.addHistogram( new TH1F( "Ctrl_WW_RedMet",";RedMet [GeV];Events",50,0,300) );
  mon.addHistogram( new TH1F( "Ctrl_WW_RedMet1",";RedMet [GeV];Events",50,0,300) );
  mon.addHistogram( new TH1F( "Ctrl_WW_RedMet2",";RedMet [GeV];Events",50,0,300) );
  mon.addHistogram( new TH1F( "Ctrl_WW_RedMet3",";RedMet [GeV];Events",50,0,300) );
  mon.addHistogram( new TH1F( "Ctrl_WW_RedMet4",";RedMet [GeV];Events",50,0,300) );
  mon.addHistogram( new TH1F( "Ctrl_WW_RedMet5",";RedMet [GeV];Events",50,0,300) );
  mon.addHistogram( new TH1F( "Ctrl_WW_zmass",";RedMet [GeV];Events",50,0,200) );

  TString jetTypes[]={"pf"};//,"pfchs"};
  const size_t nJetTypes=sizeof(jetTypes)/sizeof(TString);
  TString jetRegs[]={"TK","HEin","HEout","HF"};
  const size_t nJetRegs=sizeof(jetRegs)/sizeof(TString);
  TString btagAlgos[]={"TCHE","CSV","JP"};
  Double_t btagAlgoMin[]={-5,0,0};
  Double_t btagAlgoMax[]={15,1.,3};
  //TString jetIds[]={"pfloose","pftight","minloose","minmedium"};
  //const size_t nJetIds=sizeof(jetIds)/sizeof(TString);
  //Double_t jetPtBins[]={0,15,20,25,30,40,50,60,70,80,100,200,300,400,500,600,700,1000};
  //Int_t nJetPtBins=sizeof(jetPtBins)/sizeof(Double_t)-1;
  //Double_t jetEtaBins[]={0,0.25,0.5,0.75,1.0,1.25,1.5,1.75,2.0,2.25,2.625,2.75,2.875,3.0,3.5,4.0,4.5,5.0};
  //Double_t nJetEtaBins=sizeof(jetEtaBins)/sizeof(Double_t)-1;
  for(size_t i=0; i<nJetTypes; i++)
    {
      for(size_t ireg=0; ireg<nJetRegs; ireg++)
	{
	  mon.addHistogram( new TH1F(jetRegs[ireg]+jetTypes[i]+"jetbeta"    , ";#beta;Events",50,0,1) );
	  // 	  mon.addHistogram( new TH1F(jetRegs[ireg]+jetTypes[i]+"jetbetastar", ";#beta *;Events",50,0,1) );
	  // 	  mon.addHistogram( new TH1F(jetRegs[ireg]+jetTypes[i]+"jetdrmean"  , ";<#Delta R>;Events",50,0,0.5) );
	  // 	  mon.addHistogram( new TH1F(jetRegs[ireg]+jetTypes[i]+"jetptd"     , ";p_{T}D [GeV/c];Events",50,0,1) );
	  // 	  mon.addHistogram( new TH1F(jetRegs[ireg]+jetTypes[i]+"jetptrms"   , ";RMS p_{T} [GeV/c];Events",50,0,0.5) );
	  mon.addHistogram( new TH1F(jetRegs[ireg]+jetTypes[i]+"jetmva"     , ";MVA;Events",50,-1,1) );
	}
      for(size_t ibtag=0; ibtag<3; ibtag++)
	{
	  mon.addHistogram( new TH1F(btagAlgos[ibtag]+"b"+jetTypes[i]+"jetstags",     ";b tags;Events",50,btagAlgoMin[ibtag],btagAlgoMax[ibtag]) );
	  mon.addHistogram( new TH1F(btagAlgos[ibtag]+"other"+jetTypes[i]+"jetstags", ";udscg tags;Events",50,btagAlgoMin[ibtag],btagAlgoMax[ibtag]) );
	  mon.addHistogram( new TH1F(btagAlgos[ibtag]+jetTypes[i]+"jetstags",         ";"+btagAlgos[ibtag]+";Events",50,btagAlgoMin[ibtag],btagAlgoMax[ibtag]) );
	  mon.addHistogram( new TH1F("n"+jetTypes[i]+"jetsbtags"+btagAlgos[ibtag],    ";b-tag multiplicity ("+btagAlgos[ibtag] +");Events",5,0,5) );
	  mon.addHistogram( new TH1F("n"+jetTypes[i]+"jetsbtags"+btagAlgos[ibtag]+"NM1",    ";b-tag multiplicity ("+btagAlgos[ibtag] +");Events",5,0,5) );
	}
     
      mon.addHistogram( new TH1F(jetTypes[i]+"jetpt"       , ";p_{T} [GeV/c];Events",50,0,250) );
      mon.addHistogram( new TH1F(jetTypes[i]+"jeteta"       , ";|#eta|;Events",25,0,5) );
      mon.addHistogram( new TH2F("n"+jetTypes[i]+"jetsvspu",          ";Pileup interactions;Jet multiplicity (p_{T}>30 GeV/c);Events",50,0,50,5,0,5) );
      mon.addHistogram( new TH2F("n"+jetTypes[i]+"jetstightvspu",     ";Pileup interactions;Jet multiplicity (p_{T}>30 GeV/c);Events",50,0,50,5,0,5) );
      TH1 *h=mon.addHistogram( new TH1F("n"+jetTypes[i]+"jets",  ";Jet multiplicity (p_{T}>30 GeV/c);Events",5,0,5) );
      for(int ibin=1; ibin<=h->GetXaxis()->GetNbins(); ibin++)
	{
	  TString label("");
	  if(ibin==h->GetXaxis()->GetNbins()) label +="#geq";
	  else                                label +="=";
	  label += (ibin-1);
	  h->GetXaxis()->SetBinLabel(ibin,label);
	} 
      mon.addHistogram( new TH1F("n"+jetTypes[i]+"jets_pres",  ";Jet multiplicity after preselection (p_{T}>30 GeV/c);Events",5,0,5) );
      mon.addHistogram( new TH1F("n"+jetTypes[i]+"jetsNM1",    ";Jet multiplicity (p_{T}>30 GeV/c);Events",5,0,5) );
      mon.addHistogram( new TH2F("n"+jetTypes[i]+"jetspuidloosevspu", ";Pileup interactions;Jet multiplicity (p_{T}>30 GeV/c);Events",50,0,50,5,0,5) );
      mon.addHistogram( new TH2F("n"+jetTypes[i]+"jetspuidmediumvspu",";Pileup interactions;Jet multiplicity (p_{T}>30 GeV/c);Events",50,0,50,5,0,5) );  

//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcandjeteta"       , ";#eta;Jets",50,0,5) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcandjetdeta"       , ";|#Delta #eta|;Jets",50,0,10) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcandjetpt"       , ";p_{T} [GeV/c];Jets",50,0,250) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfhardpt"       , ";Hard p_{T} [GeV/c];Events",25,0,250) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfhardptfinal"       , ";Hard p_{T} [GeV/c];Events",25,0,250) );
//       h=mon.addHistogram( new TH1F(jetTypes[i]+"vbfcjv"       , ";Central jet count;Events",3,0,3) );
//       h->GetXaxis()->SetBinLabel(1,"=0 jets");
//       h->GetXaxis()->SetBinLabel(2,"=1 jets");
//       h->GetXaxis()->SetBinLabel(3,"#geq 2 jets");
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfhtcjv"       , ";Central jet H_{T} [GeV/c];Events",50,0,250) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfpremjj"       , ";M(jet_{1},jet_{2}) [GeV/c^{2}];Events",40,0,2000) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfmjj"       , ";M(jet_{1},jet_{2}) [GeV/c^{2}];Events",40,0,2000) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcandjetdphi"       , ";#Delta#phi;Events",20,0,3.5) );
//       mon.addHistogram( new TH2F(jetTypes[i]+"vbfmjjvsdeta"       , ";M(jet_{1},jet_{2}) [GeV/c^{2}];|#Delta #eta|;Events",40,0,2000,25,0,10) );
//       mon.addHistogram( new TH2F(jetTypes[i]+"vbfmjjvshardpt"       , ";M(jet_{1},jet_{2}) [GeV/c^{2}];Hard p_{T} [GeV/c];Events",40,0,2000,25,0,250) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcount_total"     , ";#generated vertices;Events",35,0,35) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcount_tag"       , ";#generated vertices;Events",35,0,35) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcount_tagzpt"    , ";#generated vertices;Events",35,0,35) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcount_tagzptpuid", ";#generated vertices;Events",35,0,35) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcount_tagzpttk0" , ";#generated vertices;Events",35,0,35) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcount_tagzpttk1" , ";#generated vertices;Events",35,0,35) );
//       mon.addHistogram( new TH1F(jetTypes[i]+"vbfcount_tagzpttk2" , ";#generated vertices;Events",35,0,35) );

    }

  //MET control
  mon.addHistogram( new TH1F( "mindphilmet", ";min(#Delta#phi(lepton,E_{T}^{miss});Events",40,0,4) );
  mon.addHistogram( new TH1F( "maxdphilmet", ";max(#Delta#phi(lepton,E_{T}^{miss});Events",40,0,4) );
  mon.addHistogram( new TH1F( "mindphijmet_0", ";min #Delta#phi(jet,E_{T}^{miss});Events",40,0,4) );
  mon.addHistogram( new TH1F( "mindphijmet_25", ";min #Delta#phi(jet,E_{T}^{miss});Events",40,0,4) );
  mon.addHistogram( new TH1F( "mindphijmet_50", ";min #Delta#phi(jet,E_{T}^{miss});Events",40,0,4) );
  mon.addHistogram( new TH1F( "mindphijmet", ";min #Delta#phi(jet,E_{T}^{miss});Events",40,0,4) );
  mon.addHistogram( new TH1F( "mindphijmetNM1", ";min #Delta#phi(jet,E_{T}^{miss});Events",40,0,4) );
  mon.addHistogram( new TH1F( "mindphichsjmetNM1", ";min #Delta#phi(CHS jet,E_{T}^{miss});Events",40,0,4) );
  mon.addHistogram( new TH1D( "balance",    ";E_{T}^{miss}/q_{T};Events", 25,0,2.5) );
  mon.addHistogram( new TH1D( "balanceNM1", ";E_{T}^{miss}/q_{T};Events", 25,0,2.5) );
  mon.addHistogram( new TH2D( "met_mindphilmet"  , ";E_{T}^{miss};min(#Delta#phi(lepton,E_{T}^{miss});Events", 50,0,250,40,0,4) );
  mon.addHistogram( new TH2D( "metoverlpt_mindphilmet"  , ";E_{T}^{miss}/p_{T}^{lepton};min(#Delta#phi(lepton,E_{T}^{miss});Events", 50,0,2,40,0,4) );
  mon.addHistogram( new TH1F( "met_metSB",        ";E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_met",          ";E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_metNM1",          ";Reduced E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_redMetNM1",       ";Reduced E_{T}^{miss} CHS;Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_redMetCHSNM1",    ";E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH2F( "metvsmt",          ";E_{T}^{miss};M_{T};Events", 100,0,500,200,0,1000) );
  mon.addHistogram( new TH2F( "typeImetvstypeImt",          ";type I E_{T}^{miss};type I M_{T};Events", 100,0,500,200,0,1000) );
  mon.addHistogram( new TH1F( "met_metbeforedphilmet",          ";E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_met_blind",    ";E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_mvamet",       ";MVA E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_mvamet_blind",       ";MVA E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_typeImet",     ";Type I E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_typeImet_blind",     ";Type I E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_fulltypeImet", ";Type I E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_fulltypeImetbeforedphilmet", ";Type I E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_fulltypeImet_blind", ";Type I E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_redMet",       ";Reduced E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_redMet_blind",       ";Reduced E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_redMetL",      ";Reduced E_{T}^{miss};Events", 50,-250,250) );
  mon.addHistogram( new TH1F( "met_redMetT",      ";Reduced E_{T}^{miss}.;Events", 50,-250,250) );
  mon.addHistogram( new TH1F( "met_met3leptons",  ";E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH2F( "met_met_vspu",       ";Pileup events; E_{T}^{miss};Events", 50,0,50,50,0,250) );
  mon.addHistogram( new TH2F( "met_mvamet_vspu",    ";Pileup events; MVA E_{T}^{miss};Events", 50,0,50,50,0,250) );
  mon.addHistogram( new TH2F( "met_redMet_vspu",    ";Pileup events; Reduced E_{T}^{miss};Events", 50,0,50,50,0,250) );
  mon.addHistogram( new TH2F( "met_typeImet_vspu",  ";Pileup events; Type I E_{T}^{miss};Events", 50,0,50,50,0,250) );
  mon.addHistogram( new TH2F( "met_redMetCHS_vspu", ";Pileup events; Type I E_{T}^{miss};Events", 50,0,50,50,0,250) );
  mon.addHistogram( new TH1F( "mt"  , ";M_{T};Events", 100,0,1000) );
  mon.addHistogram( new TH1F( "mtNM1"  , ";M_{T};Events", 100,0,1000) );
  mon.addHistogram( new TH1F( "mt_blind"  , ";M_{T};Events", 100,0,1000) );
  mon.addHistogram( new TH1F( "typeImt"  , ";M_{T};Events", 100,0,1000) );

  // Final distributions
  mon.addHistogram( new TH1F( "mt_final"  , ";M_{T};Events", 25,0,1000) );
  mon.addHistogram( new TH1F( "zpt_final", ";p_{T}^{ll};Events", 25,0,500) );
  mon.addHistogram( new TH1F( "met_met_final"  , ";E_{T}^{miss};Events", 25,0,250) );
  mon.addHistogram( new TH1F( "met_redMet_final"  , ";Reduced E_{T}^{miss};Events", 50,0,500) );
  mon.addHistogram( new TH1F( "met_redMetL_final"  , ";Longitudinal Reduced E_{T}^{miss};Events", 25,-100,400) );

  //MVAMET
  /*
  mon.addHistogram( new TH1F( "MVAMET_PFMet_pt"  , "PF MET ;Events", 100,0,500) );
  mon.addHistogram( new TH1F( "MVAMET_RedMet_pt"  , "Red MET ;Events", 100,0,500) );
  mon.addHistogram( new TH1F( "MVAMET_MVAMet_pt"  , "MVA MET ;Events", 100,0,500) );
  mon.addHistogram( new TH1F( "MVAMET_PFMet_bv"  , "PF MET ;Events", 100,0,500) );
  mon.addHistogram( new TH1F( "MVAMET_RedMet_bv"  , "Red MET ;Events", 100,0,500) );
  mon.addHistogram( new TH1F( "MVAMET_MVAMet_bv"  , "MVA MET ;Events", 100,0,500) );
  mon.addHistogram( new TH1F( "MVAMET_PFMet_misrec"  , "PF MET ;Events", 100,0,500) );
  mon.addHistogram( new TH1F( "MVAMET_RedMet_misrec"  , "Red MET ;Events", 100,0,500) );
  mon.addHistogram( new TH1F( "MVAMET_MVAMet_misrec"  , "MVA MET ;Events", 100,0,500) );

  mon.addHistogram( new TH1F( "MVAMET_Eff_mvamet"  , "MVA MET ;Events", 35,0.5,35.5) );
  mon.addHistogram( new TH1F( "MVAMET_Eff_pfmet"  , "MVA MET ;Events", 35,0.5,35.5) );
  mon.addHistogram( new TH1F( "MVAMET_Eff_redmet"  , "MVA MET ;Events", 35,0.5,35.5) );
  mon.addHistogram( new TH1F( "MVAMET_Eff_tot"  , "MVA MET ;Events", 35,0.5,35.5) );

  TH1F *h_MVAMETpt_mvamet_nvtx = new TH1F("h_MVAMETpt_mvamet_nvtx","",50,0.5,50.5);
  TH1F *h_MVAMETpt_pfmet_nvtx = new TH1F("h_MVAMETpt_pfmet_nvtx","",50,0.5,50.5);
  TH1F *h_MVAMETpt_redmet_nvtx = new TH1F("h_MVAMETpt_redmet_nvtx","",50,0.5,50.5);
  TH1F *h_MVAMETpt_nvtx_tot = new TH1F("h_MVAMETpt_nvtx_tot","",50,0.5,50.5);
  TH1F *h_MVAMETbv_mvamet_nvtx = new TH1F("h_MVAMETbv_mvamet_nvtx","",50,0.5,50.5);
  TH1F *h_MVAMETbv_pfmet_nvtx = new TH1F("h_MVAMETbv_pfamet_nvtx","",50,0.5,50.5);
  TH1F *h_MVAMETbv_redmet_nvtx = new TH1F("h_MVAMETbv_redmet_nvtx","",50,0.5,50.5);
  TH1F *h_MVAMETbv_nvtx_tot = new TH1F("h_MVAMETbv_nvtx_tot","",50,0.5,50.5);
  */

  //optimization
   std::vector<double> optim_Cuts1_met; 
   std::vector<double> optim_Cuts1_zpt;
   std::vector<double> optim_Cuts1_zmass;
   //std::vector<double> optim_Cuts1_jetthr;
   for(double met=50;met<140;met+=5.0) {
     if(met>80 && int(met)%10!=0) continue;
     for(double pt=30;pt<100;pt+=5) {
       if(pt>60 && int(pt)%10!=0) continue;
       for(double zm=5;zm<20;zm+=2.5) {
	 if(zm>10 && int(2*zm)%5!=0) continue;
	 //for(double jetpt=10; jetpt<20; jetpt+=5) {
	 optim_Cuts1_met   .push_back(met);
	 optim_Cuts1_zpt   .push_back(pt);
	 optim_Cuts1_zmass .push_back(zm);
	 //optim_Cuts1_jetthr.push_back(jetpt);
	 //}
       }
     }
   }

   //make it as a TProfile so hadd does not change the value
   TProfile* Hoptim_cuts1_met    =  (TProfile*) mon.addHistogram( new TProfile ("optim_cut1_met",    ";cut index;met",     optim_Cuts1_met.size(),0,optim_Cuts1_met.size()) ) ;
   TProfile* Hoptim_cuts1_zpt    =  (TProfile*) mon.addHistogram( new TProfile ("optim_cut1_zpt",    ";cut index;zpt",     optim_Cuts1_met.size(),0,optim_Cuts1_met.size()) ) ;
   TProfile* Hoptim_cuts1_zmass  =  (TProfile*) mon.addHistogram( new TProfile ("optim_cut1_zm",     ";cut index;zmass",   optim_Cuts1_met.size(),0,optim_Cuts1_met.size()) ) ;
   //TProfile* Hoptim_cuts1_jetthr =  (TProfile*) mon.addHistogram( new TProfile ("optim_cut1_jetthr", ";cut index;jet thr", optim_Cuts1_met.size(),0,optim_Cuts1_met.size()) ) ;
   for(unsigned int index=0;index<optim_Cuts1_met.size();index++){
      Hoptim_cuts1_met   ->Fill(index, optim_Cuts1_met[index]);    
      Hoptim_cuts1_zpt   ->Fill(index, optim_Cuts1_zpt[index]);
      Hoptim_cuts1_zmass ->Fill(index, optim_Cuts1_zmass[index]);
      //Hoptim_cuts1_jetthr->Fill(index, optim_Cuts1_jetthr[index]);
   }

   TH1F* Hoptim_systs     =  (TH1F*) mon.addHistogram( new TH1F ("optim_systs"    , ";syst;", nvarsToInclude,0,nvarsToInclude) ) ;
   for(size_t ivar=0; ivar<nvarsToInclude; ivar++)
   {
     mon.addHistogram( new TH1F( "zptvar"+varNames[ivar], ";p_{T}^{ll} [GeV/c];Events", 100,0,1000) );
     mon.addHistogram( new TH1F( "mtvar"+varNames[ivar],  ";M_{T} [GeV/c^{2}];Events", 100,0,1000) );
     
     Hoptim_systs->GetXaxis()->SetBinLabel(ivar+1, varNames[ivar]);
     mon.addHistogram( new TH2F (TString("mt_shapes")+varNames[ivar],";cut index;M_{T} [GeV/c^{2}];#events (/10GeV)",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 160,150,950) );
     //     mon.addHistogram( new TH2F (TString("mt_mvaMet_shapes")+varNames[ivar],";cut index;M_{T} [GeV/c^{2}];#events (/10GeV)",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 160,150,950) );
     //     mon.addHistogram( new TH2F (TString("mt_shapesZ10")+varNames[ivar],";cut index;M_{T} [GeV/c^{2}];#events (/25GeV)",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 1,150,950) );//only cut&count
     //     mon.addHistogram( new TH2F (TString("mt_shapesZ5")+varNames[ivar],";cut index;M_{T} [GeV/c^{2}];#events (/25GeV)",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 1,150,950) );//only cut&count
     
     //3lepton SB
     //     mon.addHistogram( new TH2F (TString("mt_shapes_3rdLepton")+varNames[ivar],";cut index;M_{T} [GeV/c^{2}];",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 1,150,950) );
     
     //     if(ivar==0)mon.addHistogram( new TH2F (TString("mt_shapesBTagSB")+varNames[ivar],";cut index;M_{T} [GeV/c^{2}];",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 160,150,950) );
     mon.addHistogram( new TH2F (TString("mt_redMet_shapes")+varNames[ivar],";cut index;M_{T} [GeV/c^{2}];",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 160,150,950) );
     
     //     mon.addHistogram( new TH2F (TString("mt3")+varNames[ivar],";cut index;M_{T}^{3rd lepton} [GeV/c^{2}];",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 50,0,250) );
     TH2F *h=(TH2F *) mon.addHistogram( new TH2F ("mt_shapes_NRBctrl"+varNames[ivar],";cut index;Selection region;Events",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(),6,0,6) );
     h->GetYaxis()->SetBinLabel(1,"M_{in}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(2,"M_{out}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(3,"M_{out+}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(4,"M_{in}^{ll}/#geq 1 b-tag");
     h->GetYaxis()->SetBinLabel(5,"M_{out}^{ll}/#geq 1 b-tag");
     h->GetYaxis()->SetBinLabel(6,"M_{out+}^{ll}/#geq 1 b-tag");

     //      h=(TH2F *) mon.addHistogram( new TH2F ("mt_mvaMet_shapes_NRBctrl"+varNames[ivar],";cut index;Selection region;Events",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(),6,0,6) );
     //      h->GetYaxis()->SetBinLabel(1,"M_{in}^{ll}/=0 b-tags");
     //      h->GetYaxis()->SetBinLabel(2,"M_{out}^{ll}/=0 b-tags");
     //      h->GetYaxis()->SetBinLabel(3,"M_{out+}^{ll}/=0 b-tags");
     //      h->GetYaxis()->SetBinLabel(4,"M_{in}^{ll}/#geq 1 b-tag");
     //      h->GetYaxis()->SetBinLabel(5,"M_{out}^{ll}/#geq 1 b-tag");
     //      h->GetYaxis()->SetBinLabel(6,"M_{out+}^{ll}/#geq 1 b-tag");
     
     h=(TH2F *) mon.addHistogram( new TH2F ("mt_redMet_shapes_NRBctrl"+varNames[ivar],";cut index;Selection region;Events",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(),6,0,6) );
     h->GetYaxis()->SetBinLabel(1,"M_{in}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(2,"M_{out}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(3,"M_{out+}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(4,"M_{in}^{ll}/#geq 1 b-tag");
     h->GetYaxis()->SetBinLabel(5,"M_{out}^{ll}/#geq 1 b-tag");
     h->GetYaxis()->SetBinLabel(6,"M_{out+}^{ll}/#geq 1 b-tag");

     // for ZZ analysis (use Zpt or MET shape)
     mon.addHistogram( new TH2F (TString("redMet_shapes")+varNames[ivar],";cut index;red-MET [GeV];#events (/5GeV)",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 160,0,800) );
     mon.addHistogram( new TH2F (TString("zpt_shapes")+varNames[ivar],";cut index;Z p_{T} [GeV/c];#events (/5GeV)",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(), 160,0,800) );
     
     h=(TH2F *) mon.addHistogram( new TH2F ("redMet_shapes_NRBctrl"+varNames[ivar],";cut index;Selection region;Events",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(),6,0,6) );
     h->GetYaxis()->SetBinLabel(1,"M_{in}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(2,"M_{out}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(3,"M_{out+}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(4,"M_{in}^{ll}/#geq 1 b-tag");
     h->GetYaxis()->SetBinLabel(5,"M_{out}^{ll}/#geq 1 b-tag");
     h->GetYaxis()->SetBinLabel(6,"M_{out+}^{ll}/#geq 1 b-tag");

     h=(TH2F *) mon.addHistogram( new TH2F ("zpt_shapes_NRBctrl"+varNames[ivar],";cut index;Selection region;Events",optim_Cuts1_met.size(),0,optim_Cuts1_met.size(),6,0,6) );
     h->GetYaxis()->SetBinLabel(1,"M_{in}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(2,"M_{out}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(3,"M_{out+}^{ll}/=0 b-tags");
     h->GetYaxis()->SetBinLabel(4,"M_{in}^{ll}/#geq 1 b-tag");
     h->GetYaxis()->SetBinLabel(5,"M_{out}^{ll}/#geq 1 b-tag");
     h->GetYaxis()->SetBinLabel(6,"M_{out+}^{ll}/#geq 1 b-tag");
   }
   
   
//    std::vector<double> optim_Cuts2_met;
//    std::vector<double> optim_Cuts2_vbfJpt;
//    std::vector<double> optim_Cuts2_vbfdeta;
//    for(double met=50;met<=80;met+=10){
//      for(double j1=20;j1<=40;j1+=5){
//        for(double deta=2.5;deta<=5.0;deta+=0.5){
// 	 optim_Cuts2_met    .push_back(met);
// 	 optim_Cuts2_vbfJpt .push_back(j1);
// 	 optim_Cuts2_vbfdeta.push_back(deta);
//        }
//      }
//    }
   
   //make it as a TProfile so hadd does not change the value
//    TProfile* Hoptim_cuts2_met     =  (TProfile*) mon.addHistogram( new TProfile ("optim_cut2_met"    , ";cut index;met"    ,optim_Cuts2_met.size(),0,optim_Cuts2_met.size()) ) ;
//    TProfile* Hoptim_cuts2_jet     =  (TProfile*) mon.addHistogram( new TProfile ("optim_cut2_jet"    , ";cut index;jet"    ,optim_Cuts2_met.size(),0,optim_Cuts2_met.size()) ) ;
//    TProfile* Hoptim_cuts2_deta    =  (TProfile*) mon.addHistogram( new TProfile ("optim_cut2_deta"   , ";cut index;deta"   ,optim_Cuts2_met.size(),0,optim_Cuts2_met.size()) ) ;
//    for(unsigned int index=0;index<optim_Cuts2_met.size();index++){
//       Hoptim_cuts2_met    ->Fill(index, optim_Cuts2_met[index]);  
//       Hoptim_cuts2_jet    ->Fill(index, optim_Cuts2_vbfJpt[index]);  
//       Hoptim_cuts2_deta   ->Fill(index, optim_Cuts2_vbfdeta[index]);
//    }

   for(size_t ivar=0; ivar<nvarsToInclude; ivar++){
     Hoptim_systs->GetXaxis()->SetBinLabel(ivar+1, varNames[ivar]);
//      mon.addHistogram( new TH2F (TString("VBFmt_shapes")+varNames[ivar],";cut index;M_{T} [GeV/c^{2}];#events (/10GeV)",optim_Cuts2_met.size(),0,optim_Cuts2_met.size(), 32,0,800) );
//      TH2F *h=(TH2F *) mon.addHistogram( new TH2F ("VBFmt_shapes_NRBctrl"+varNames[ivar],";cut index;Selection region;Events",optim_Cuts2_met.size(),0,optim_Cuts2_met.size(),6,0,6) );
//      h->GetYaxis()->SetBinLabel(1,"M_{in}^{ll}/=0 b-tags");
//      h->GetYaxis()->SetBinLabel(2,"M_{out}^{ll}/=0 b-tags");
//      h->GetYaxis()->SetBinLabel(3,"M_{out+}^{ll}/=0 b-tags");
//      h->GetYaxis()->SetBinLabel(4,"M_{in}^{ll}/#geq 1 b-tag");
//      h->GetYaxis()->SetBinLabel(5,"M_{out}^{ll}/#geq 1 b-tag");
//      h->GetYaxis()->SetBinLabel(6,"M_{out+}^{ll}/#geq 1 b-tag");
   }


  //##############################################
  //######## GET READY FOR THE EVENT LOOP ########
  //##############################################

  //open the file and get events tree
  ZZ2l2nuSummaryHandler evSummaryHandler;
  TFile *file = TFile::Open(url);
  printf("Looping on %s\n",url.Data());
  if(file==0) return -1;
  if(file->IsZombie()) return -1;
  if( !evSummaryHandler.attachToTree( (TTree *)file->Get(dirname) ) ){
      file->Close();
      return -1;
  }


  //check run range to compute scale factor (if not all entries are used)
  const Int_t totalEntries= evSummaryHandler.getEntries();
  float rescaleFactor( evEnd>0 ?  float(totalEntries)/float(evEnd-evStart) : -1 );
  if(evEnd<0 || evEnd>evSummaryHandler.getEntries() ) evEnd=totalEntries;
  if(evStart > evEnd ){
      file->Close();
      return -1;
  }

  //MC normalization (to 1/pb)
  float cnorm=1.0;
  if(isMC){
      TH1F* cutflowH = (TH1F *) file->Get("evAnalyzer/h2zz/cutflow");
      if(cutflowH) cnorm=cutflowH->GetBinContent(1);
      if(rescaleFactor>0) cnorm /= rescaleFactor;
      printf("cnorm = %f\n",cnorm);
  }
  Hcutflow->SetBinContent(1,cnorm);


  //pileup weighting: based on vtx for now...
  std::vector<double> dataPileupDistributionDouble = runProcess.getParameter< std::vector<double> >("datapileup");
  std::vector<float> dataPileupDistribution; for(unsigned int i=0;i<dataPileupDistributionDouble.size();i++){dataPileupDistribution.push_back(dataPileupDistributionDouble[i]);}
  std::vector<float> mcPileupDistribution;
  bool useObservedPU(true);
  //bool useObservedPU(use2011Id);
  if(!use2011Id && url.Contains("toZZto2L")) useObservedPU=true;
  if(isMC){
    TString puDist("evAnalyzer/h2zz/pileuptrue");
    if(useObservedPU) puDist="evAnalyzer/h2zz/pileup";
    TH1F* histo = (TH1F *) file->Get(puDist);
    if(!histo)std::cout<<"pileup histogram is null!!!\n";
    for(int i=1;i<=histo->GetNbinsX();i++){mcPileupDistribution.push_back(histo->GetBinContent(i));}
    delete histo;
    if(dataPileupDistribution.size()==0) dataPileupDistribution=mcPileupDistribution;
  }
  while(mcPileupDistribution.size()<dataPileupDistribution.size())  mcPileupDistribution.push_back(0.0);
  while(mcPileupDistribution.size()>dataPileupDistribution.size())dataPileupDistribution.push_back(0.0);

  gROOT->cd();  //THIS LINE IS NEEDED TO MAKE SURE THAT HISTOGRAM INTERNALLY PRODUCED IN LumiReWeighting ARE NOT DESTROYED WHEN CLOSING THE FILE
  edm::LumiReWeighting *LumiWeights=0;
  PuShifter_t PuShifters;
  //  reweight::PoissonMeanShifter *PShiftUp=0, *PShiftDown=0;
  if(isMC){
      LumiWeights= new edm::LumiReWeighting(mcPileupDistribution,dataPileupDistribution);
      PuShifters=getPUshifters(dataPileupDistribution,0.05);
      //      PShiftUp = new reweight::PoissonMeanShifter(+0.8);
      //      PShiftDown = new reweight::PoissonMeanShifter(-0.8);
  }

  //event Categorizer
  //EventCategory eventCategoryInst(0); //inclusive analysis
  //EventCategory eventCategoryInst(1); //jet binning
  //EventCategory eventCategoryInst(2); //vbf binning
  //  EventCategory eventCategoryInst(3); //jet(0,1,>=2)+vbf binning
  EventCategory eventCategoryInst(4); //jet(0,>=1)+vbf binning



  //##############################################
  //########           EVENT LOOP         ########
  //##############################################
  //loop on all the events
  printf("Progressing Bar     :0%%       20%%       40%%       60%%       80%%       100%%\n");
  printf("Scanning the ntuple :");
  int treeStep = (evEnd-evStart)/50;if(treeStep==0)treeStep=1;
  DuplicatesChecker duplicatesChecker;
  int nDuplicates(0);
  for( int iev=evStart; iev<evEnd; iev++){
      if((iev-evStart)%treeStep==0){printf(".");fflush(stdout);}

      //##############################################   EVENT LOOP STARTS   ##############################################
      //load the event content from tree
      evSummaryHandler.getEntry(iev);
      ZZ2l2nuSummary_t &ev=evSummaryHandler.getEvent();
      if(!isMC && duplicatesChecker.isDuplicate( ev.run, ev.lumi, ev.event) ) { nDuplicates++; continue; }
      PhysicsEvent_t phys=getPhysicsEventFrom(ev);
      bool mustBlind = (!isMC && runBlinded && evSummaryHandler.hasSpoilerAlert(!isMC));

      //event category
      bool isSameFlavor(ev.cat==MUMU || ev.cat==EE);
      TString tag_cat;
      switch(ev.cat){
         case MUMU : tag_cat = "mumu";  break;
         case EE   : tag_cat = "ee";    break;
         case EMU  : tag_cat = "emu";   break;
         default   : continue;
      }
      //      if(isMC && mctruthmode==1 && !isDYToLL(ev.mccat) && !isZZ2l2nu(ev.mccat) ) continue;
      if(isMC && mctruthmode==1 && !isDYToLL(ev.mccat) ) continue;
      if(isMC && mctruthmode==2 && !isDYToTauTau(ev.mccat) ) continue;

      //require compatibilitiy of the event with the PD
      bool hasTrigger(false);
      bool hasEEtrigger = ev.triggerType & 0x1;
      bool hasMMtrigger = (ev.triggerType >> 1 ) & 0x1;
      bool hasEMtrigger = (ev.triggerType >> 2 ) & 0x1;
      bool hasMtrigger  = (ev.triggerType >> 3 ) & 0x1;
      if(!isMC){
	  if(ev.cat!=fType) continue;

	  if(ev.cat==EE   && !hasEEtrigger) continue;
	  if(ev.cat==MUMU && !(hasMMtrigger||hasMtrigger) ) continue;
	  if(ev.cat==EMU  && !hasEMtrigger) continue;

	  //this is a safety veto for the single mu PD
	  if(isSingleMuPD) {
	    if(!hasMtrigger) continue;
	    if(hasMtrigger && hasMMtrigger) continue;
	  }

	  hasTrigger=true;
      }
      else 
	{
	  if(ev.cat==EE   && hasEEtrigger) hasTrigger=true;
	  if(ev.cat==MUMU && (hasMMtrigger || hasMtrigger) ) hasTrigger=true;
	  if(ev.cat==EMU  && hasEMtrigger) hasTrigger=true;
	}
      
      //prepare the tag's vectors for histo filling
      std::vector<TString> tags_inc(1,"all");
      std::vector<TString> tags_small(1);
      std::vector<TString> tags_cat(1,"all");
      std::vector<TString> tags_full(1,"all");

      
      //pileup weight
      float weight = 1.0;
      float noLShapeWeight=1.0;
      float signalWeight=1.0;
      double TotalWeight_plus = 1.0;
      double TotalWeight_minus = 1.0;
      float lShapeWeights[4]={1.0,1.0,1.0,1.0};



      if(isMC){

        weight            = LumiWeights->weight(useObservedPU ? ev.ngenITpu : ev.ngenTruepu);
        TotalWeight_plus  = PuShifters[PUUP]->Eval(useObservedPU ? ev.ngenITpu : ev.ngenTruepu);
        TotalWeight_minus = PuShifters[PUDOWN]->Eval(useObservedPU ? ev.ngenITpu : ev.ngenTruepu);

//         if(isMC_VBF || isMC_GG)mon.fillHisto("higgsMass_0raw",tags_inc, phys.genhiggs[0].mass(), weight);
// 	//if(isMC_VBF){ signalWeight = weightVBF(VBFString,HiggsMass, phys.genhiggs[0].mass() );  weight*=signalWeight; }
//         if(isMC_VBF || isMC_GG)mon.fillHisto("higgsMass_1vbf",tags_inc, phys.genhiggs[0].mass(), weight); 

        //if(isMC_GG) {
	for(size_t iwgt=0; iwgt<hWeightsGrVec.size(); iwgt++) 
	  ev.hptWeights[iwgt] = hWeightsGrVec[iwgt]->Eval(phys.genhiggs[0].pt());
	weight *= ev.hptWeights[0];
	  //}
//         if(isMC_VBF || isMC_GG)mon.fillHisto("higgsMass_2qt" ,tags_inc, phys.genhiggs[0].mass(), weight);
  
	for(size_t iwgt=0; iwgt<hLineShapeGrVec.size(); iwgt++)
	  lShapeWeights[iwgt]=hLineShapeGrVec[iwgt]->Eval(phys.genhiggs[0].mass());
	noLShapeWeight=weight;
	weight *= lShapeWeights[0];
        //printf("lsw=%f \n",lShapeWeights[0]);
	
//         if(isMC_VBF || isMC_GG)mon.fillHisto("higgsMass_sls" ,tags_inc, phys.genhiggs[0].mass(), weight*lShapeWeights[3]/lShapeWeights[0]);	
//         if(isMC_VBF || isMC_GG)mon.fillHisto("higgsMass_3ls" ,tags_inc, phys.genhiggs[0].mass(), weight);	
//         if(isMC_VBF || isMC_GG)mon.fillHisto("higgsMass_ls"  ,tags_inc, phys.genhiggs[0].mass(), weight/signalWeight);
      }
      Hcutflow->Fill(1,1);
      Hcutflow->Fill(2,weight);
      Hcutflow->Fill(3,weight*TotalWeight_minus);
      Hcutflow->Fill(4,weight*TotalWeight_plus);
      Hcutflow->Fill(5,signalWeight);

      //MET variables
      LorentzVector rawMetP4=phys.met[2];
      LorentzVector fullTypeIMetP4=phys.met[0];
      LorentzVector mvaMetP4=phys.met[7];

      bool useCHS(true);
      bool useJERsmearing(true); 
      bool useJetsOnlyInTracker(true); 
      bool usePUsubJetId(true); 

      // invert ajets <-> jets ...
      // 2011:  ajets -> non CHS,  jets -> CHS
      // 2012:  ajets -> CHS,      jets -> non CHS
      if(use2011Id) useCHS = !useCHS; 

      //apply JER base corrections to jets (and compute associated variations on the MET variable)
      // std PF
      std::vector<PhysicsObjectJetCollection> variedAJets;
      LorentzVectorCollection zvvs;
      PhysicsObjectJetCollection &recoJets = ( useCHS ? phys.ajets : phys.jets);
      METUtils::computeVariation(recoJets, phys.leptons, rawMetP4, variedAJets, zvvs, &jecUnc);
      if(!useJERsmearing) zvvs[0] = rawMetP4; // this stinks a bit...

      // CHS PF
      std::vector<PhysicsObjectJetCollection> variedCHSJets;
      LorentzVectorCollection CHSzvvs;
      METUtils::computeVariation(phys.ajets, phys.leptons, rawMetP4, variedCHSJets, CHSzvvs, &jecUnc);
      if(!useJERsmearing) CHSzvvs[0] = rawMetP4; // this stinks a bit...

      //
      // LEPTON ANALYSIS
      //
      LorentzVector lep1=phys.leptons[0];
      LorentzVector lep2=phys.leptons[1];
      LorentzVector zll(lep1+lep2);
      bool passId(true);
      bool passIdAndIso(true);
      //bool passZmass(fabs(zll.mass()-91)<15);
      bool passZmass(fabs(zll.mass()-91)<7.5); if(!use2011Id) passZmass = (fabs(zll.mass()-91)<7.5);
      //bool passZmass10(fabs(zll.mass()-91)<10);
      //bool passZmass5(fabs(zll.mass()-91)<5);
      bool isZsideBand( (zll.mass()>40 && zll.mass()<70) || (zll.mass()>110 && zll.mass()<200));
      bool isZsideBandPlus( (zll.mass()>110 && zll.mass()<200));
      bool passZpt(zll.pt()>40); if(!use2011Id)  passZpt = (zll.pt()>45);
      //bool passZpt(zll.pt()>55);// && fabs(zll.eta())<1.442);
      //bool passZpt(zll.pt()>30);

      //check alternative selections for the dilepton
      double llScaleFactor(1.0),llTriggerEfficiency(1.0);
      LorentzVector genZP4(0,0,0,0); // for checks on Sherpa ZZ 
      int genmatchid[2] = {-1, -1}; 
      double genmatchdr[2] = {0.1, 0.1}; 
      for(int ilep=0; ilep<2; ilep++){
	  TString lepStr( fabs(phys.leptons[ilep].id)==13 ? "mu" : "e");
	  
	  //generator level matching
	  //  int matchid(0);
	  //LorentzVector genP4(0,0,0,0);
	  for(size_t igl=0;igl<phys.genleptons.size(); igl++) 
	    {
	      //if(deltaR(phys.genleptons[igl],phys.leptons[ilep])>0.1) continue;
	      if(ilep==1 && int(igl)==genmatchid[0]) continue; 
	      if(deltaR(phys.genleptons[igl],phys.leptons[ilep])<genmatchdr[ilep]) {
		genmatchdr[ilep] = deltaR(phys.genleptons[igl],phys.leptons[ilep]);
		genmatchid[ilep] = igl; 
	      }
	    }
	  if(genmatchid[0]>-1 && genmatchid[1]>-1) {
	    //genP4=phys.genleptons[igl];
	    //matchid=phys.genleptons[igl].id;
	    genZP4 = phys.genleptons[0] + phys.genleptons[1];
	  }

	  //id and isolation
	  int lpid=phys.leptons[ilep].pid;
	  float relIso2011    = phys.leptons[ilep].relIsoRho(ev.rho);
	  float relIso = (lepStr=="mu") ? 
	    phys.leptons[ilep].pfRelIsoDbeta() :
	    phys.leptons[ilep].ePFRelIsoCorrected2012(ev.rho);
	  std::vector<int> passIds;
	  std::map<int,bool> passIsos;
	  bool hasGoodId(false), isIso(false);
	  if(fabs(phys.leptons[ilep].id)==13)
	    {
	      if( hasObjectId(ev.mn_idbits[lpid], MID_LOOSE) )    { passIds.push_back(0); passIsos[0]=(relIso<0.2); }
	      if( hasObjectId(ev.mn_idbits[lpid], MID_TIGHT) )    { passIds.push_back(1); passIsos[1]=(relIso<0.2); if(!use2011Id) { hasGoodId=true; isIso=passIsos[0]; } }
	      if( hasObjectId(ev.mn_idbits[lpid], MID_VBTF2011) ) { passIds.push_back(2); passIsos[2]=(relIso2011<0.15); if(use2011Id) {hasGoodId=true; isIso=passIsos[2];} }
	      if( hasObjectId(ev.mn_idbits[lpid], MID_SOFT) )     { passIds.push_back(3); passIsos[3]=true;}
	      if(use2011Id) 
		{
		  try{
		    llScaleFactor *= muonScaleFactor(phys.leptons[ilep].pt(),fabs(phys.leptons[ilep].eta()));
		    llTriggerEfficiency *= muonTriggerEfficiency(phys.leptons[ilep].pt(),fabs(phys.leptons[ilep].eta()));
		  }catch(std::string &e){
		  }
		}
	      else
		{
		  llScaleFactor *= 1;
		  llTriggerEfficiency *= 1.0; //muonTriggerEfficiency(phys.leptons[ilep].pt(),fabs(phys.leptons[ilep].eta()),2012);
		}
	    }
	  else
	    {
	      int wps[]={EgammaCutBasedEleId::LOOSE,EgammaCutBasedEleId::MEDIUM, EID_VBTF2011, EgammaCutBasedEleId::VETO};
	      for(int iwp=0; iwp<4; iwp++)
		{
		  if(iwp==2 && hasObjectId(ev.en_idbits[lpid], EID_VBTF2011)) 
		    { 
		      passIds.push_back(2); passIsos[2]=(relIso2011<0.10); 
		      if(use2011Id) 
			{ 
			  hasGoodId=true; isIso=passIsos[2]; 
			  try{
			    llScaleFactor *= electronScaleFactor(phys.leptons[ilep].pt(),fabs(phys.leptons[ilep].eta()));
			    llTriggerEfficiency *= electronTriggerEfficiency(phys.leptons[ilep].pt(),fabs(phys.leptons[ilep].eta()));
			  } catch(std::string &e){
			  }
			}
		    }
		  else
		    {
		      bool passWp = EgammaCutBasedEleId::PassWP(EgammaCutBasedEleId::WorkingPoint(wps[iwp]),
								(fabs(phys.leptons[ilep].eta())<1.4442),
								phys.leptons[ilep].pt(), phys.leptons[ilep].eta(),
								ev.en_detain[lpid],  ev.en_dphiin[lpid], ev.en_sihih[lpid], ev.en_hoe[lpid],
								ev.en_ooemoop[lpid], phys.leptons[ilep].d0, phys.leptons[ilep].dZ,
								0., 0., 0.,
								!hasObjectId(ev.en_idbits[lpid], EID_CONVERSIONVETO),0,ev.rho);
		      if(passWp) { 
			passIds.push_back(iwp); 
			passIsos[iwp]=(relIso<0.15);
			if(wps[iwp]==EgammaCutBasedEleId::MEDIUM && !use2011Id){  hasGoodId=true; isIso=passIsos[iwp]; }
		      }
		      if(!use2011Id)
			{
			  llScaleFactor *= 1;
			  llTriggerEfficiency *= 1.0; //electronTriggerEfficiency(phys.leptons[ilep].pt(),fabs(phys.leptons[ilep].eta()),2012);
			}
		    }
		}
	    }
	  if(!hasGoodId)  { passId=false; passIdAndIso=false; }
	  if(!isIso) passIdAndIso=false;     
	  
	  //        fill control histograms (constrained to the Z mass)
	  if(passZmass && isSameFlavor){
	    // 	      if(matchid!=0){
	    // 		  mon.fillHisto(lepStr+"genpt",tags_inc, genP4.pt(), weight,true);
	    // 		  mon.fillHisto(lepStr+"geneta",tags_inc, genP4.eta(), weight);
	    // 		  mon.fillHisto(lepStr+"genpu",tags_inc,ev.ngenITpu, weight);
	    // 		  for(size_t iid=0; iid<passIds.size(); iid++){
	    // 		      TString idStr(lepStr);  idStr += passIds[iid];
	    // 		      mon.fillHisto(idStr+"pt",tags_inc, genP4.pt(), weight,true);
	    // 		      mon.fillHisto(idStr+"eta",tags_inc, genP4.eta(), weight);
	    // 		      mon.fillHisto(idStr+"pu",tags_inc,ev.ngenITpu, weight);
	    // 		      if(!passIsos[ passIds[iid] ]) continue;
	    // 		      mon.fillHisto(idStr+"isopt",tags_inc,  genP4.pt(), weight,true);
	    // 		      mon.fillHisto(idStr+"isoeta",tags_inc, genP4.eta(), weight);
	    // 		      mon.fillHisto(idStr+"isopu",tags_inc,ev.ngenITpu, weight);
	    // 		    }
	    // 	      }
	    
	    // 	      mon.fillHisto(lepStr+"d0",              tags_inc,fabs(phys.leptons[ilep].d0),weight);
	    // 	      mon.fillHisto(lepStr+"dZ",              tags_inc,fabs(phys.leptons[ilep].dZ),weight);
	    // 	      mon.fillHisto(lepStr+"trkchi2",         tags_inc,fabs(phys.leptons[ilep].trkchi2),weight);
	    // 	      mon.fillHisto(lepStr+"trkvalidpixel",   tags_inc,fabs(phys.leptons[ilep].trkValidPixelHits),weight);
	    // 	      mon.fillHisto(lepStr+"trkvalidtracker", tags_inc,fabs(phys.leptons[ilep].trkValidTrackerHits),weight);
	    // 	      mon.fillHisto(lepStr+"losthits",        tags_inc,fabs(phys.leptons[ilep].trkLostInnerHits),weight);
	    
	    // 	      if(lepStr=="e"){
	    // 		  TString reg="ee";
	    // 		  if(fabs(phys.leptons[ilep].eta())<1.442) reg="eb";
	    // 		  mon.fillHisto(lepStr+reg+"detain",  tags_inc,fabs(ev.en_detain[lpid]),weight);
	    // 		  mon.fillHisto(lepStr+reg+"dphiin",  tags_inc,fabs(ev.en_dphiin[lpid]),weight);
	    // 		  mon.fillHisto(lepStr+reg+"sihih",   tags_inc,fabs(ev.en_sihih[lpid]),weight);
	    // 		  mon.fillHisto(lepStr+reg+"sipip",   tags_inc,fabs(ev.en_sipip[lpid]),weight);
	    // 		  mon.fillHisto(lepStr+reg+"r9",      tags_inc,fabs(ev.en_r9[lpid]),weight);
	    // 		  mon.fillHisto(lepStr+reg+"hoe",     tags_inc,fabs(ev.en_hoe[lpid]),weight);
	    // 		  mon.fillHisto(lepStr+reg+"eopin",   tags_inc,fabs(ev.en_eopin[lpid]),weight);
	    // 		  mon.fillHisto(lepStr+reg+"fbrem",   tags_inc,fabs(ev.en_fbrem[lpid]),weight);
	    // 		  mon.fillHisto(lepStr+reg+"ooemoop", tags_inc,fabs(ev.en_ooemoop[lpid]),weight);
	    // 	      }else{
	    // 		  mon.fillHisto(lepStr+"nmuonhits",  tags_inc,fabs(ev.mn_validMuonHits[lpid]),weight);
	    // 		  mon.fillHisto(lepStr+"nmatches",   tags_inc,fabs(ev.mn_nMatches[lpid]),weight);
	    // 	      }
	    
	    if(hasGoodId){
	      mon.fillHisto(lepStr+"reliso",     tags_inc, use2011Id? relIso2011 : relIso,   weight);
	      // 	      TString lepType(matchid!=0 ? "true":"fake");
	      // 	      if(isMC){
	      // 		mon.fillHisto(lepStr+lepType+"reliso",     tags_inc, use2011id ? relIso2011: relIso,   weight);
	      // 	      }
	    }
          }
      }
      
      tags_full.push_back(tag_cat);
      tags_cat.push_back(tag_cat);
      if(tag_cat=="mumu" || tag_cat=="ee"){tags_full.push_back("ll"); tags_cat.push_back("ll"); tags_small.push_back("ll");}


      //
      // 3rd LEPTON ANALYSIS
      //
      bool pass3dLeptonVeto(true);
      int nextraleptons(0);
      std::vector<LorentzVector> extraLeptonsP4;
      for(size_t ilep=2; ilep<phys.leptons.size(); ilep++){
	//lepton type
	bool isGood(false);
	int lpid=phys.leptons[ilep].pid;
	if(fabs(phys.leptons[ilep].id)==13){
	  if(!use2011Id){
	    isGood = (hasObjectId(ev.mn_idbits[lpid], MID_LOOSE) && phys.leptons[ilep].pfRelIsoDbeta()<0.2);
	    isGood |= (hasObjectId(ev.mn_idbits[lpid], MID_SOFT) && phys.leptons[ilep].pt()>3);
	  }else{
	    isGood = (hasObjectId(ev.mn_idbits[lpid], MID_VBTF2011) && phys.leptons[ilep].relIsoRho(ev.rho)<0.15 && phys.leptons[ilep].pt()>10);
	    isGood |= (hasObjectId(ev.mn_idbits[lpid], MID_SOFT2011) && phys.leptons[ilep].pt()>3);
	  }
	}else{
	  if(!use2011Id){
	    isGood = ( hasObjectId(ev.en_idbits[lpid],EID_VETO) && phys.leptons[ilep].ePFRelIsoCorrected2012(ev.rho)<0.15 && phys.leptons[ilep].pt()>10);
	  }else{
	    isGood = ( hasObjectId(ev.en_idbits[lpid],EID_VBTF2011) && phys.leptons[ilep].relIsoRho(ev.rho)<0.1 && phys.leptons[ilep].pt()>10);
	  }
	}
	nextraleptons += isGood;
	
	if(!isGood) continue;
	extraLeptonsP4.push_back( phys.leptons[ilep] );
      }
      pass3dLeptonVeto=(nextraleptons==0);
      

      //
      //STD PF JET ANALYSIS
      //
      bool passJetveto(true);
      bool passBveto(true);
      bool passRedMet(true);
      bool passDphijmet(true);
      bool passBalanceCut(true);
      //PhysicsObjectJetCollection aJets= variedAJets[0];
      PhysicsObjectJetCollection &aJets = ( useJERsmearing ? variedAJets[0] : recoJets );
      PhysicsObjectJetCollection aGoodIdJets;
      LorentzVector aClusteredMetP4(zll); aClusteredMetP4 *= -1;
      int nAJetsLoose(0), nAJetsTight(0), nAJetsPUIdLoose(0), nAJetsPUIdMedium(0), nAJetsGood30(0);
      int nABtags[3]={0,0,0};
      float mindphijmet(999999.),mindphijmet15(999999.);
      PhysicsObjectJetCollection recoilJets;
      for(size_t ijet=0; ijet<aJets.size(); ijet++) 
	{
	  float idphijmet( fabs(deltaPhi(aJets[ijet].phi(),zvvs[0].phi()) ) );
	  if(aJets[ijet].pt()>15) if(idphijmet<mindphijmet15)  mindphijmet15=idphijmet;
	  if(aJets[ijet].pt()>30) if(idphijmet<mindphijmet)  mindphijmet=idphijmet;
	  if(fabs(deltaPhi(aJets[ijet].phi(),zll.phi()))>2) recoilJets.push_back( aJets[ijet] );
	  
	  bool isGoodJet = hasObjectId(aJets[ijet].pid,JETID_LOOSE); // TIGHT);
	  if(usePUsubJetId) isGoodJet = hasObjectId(aJets[ijet].pid,JETID_CUTBASED_LOOSE); 
	  TString reg      = getJetRegion(aJets[ijet].eta());
 	  mon.fillHisto(reg+"pfjetbeta",     tags_inc,aJets[ijet].beta,     weight);
	  // 	  mon.fillHisto(reg+"pfjetbetastar", tags_inc,aJets[ijet].betaStar, weight);
	  // 	  mon.fillHisto(reg+"pfjetdrmean",   tags_inc,aJets[ijet].dRMean,   weight);
	  // 	  mon.fillHisto(reg+"pfjetptd",      tags_inc,aJets[ijet].ptD,      weight);
	  // 	  mon.fillHisto(reg+"pfjetptrms",    tags_inc,aJets[ijet].ptRMS,    weight);
	  mon.fillHisto(reg+"pfjetmva",      tags_inc,aJets[ijet].pumva,    weight);
	  if(isGoodJet)
	    {
	      aClusteredMetP4 -= aJets[ijet];	  

	      if(useJetsOnlyInTracker && fabs(aJets[ijet].eta())>2.5) continue; 
	      aGoodIdJets.push_back(aJets[ijet]);
	      if(aJets[ijet].pt()>30)nAJetsGood30++;

	      //if(aJets[ijet].pt()<30) continue; // ???????????????????????????
	      if(aJets[ijet].pt()<20) continue; 
	      if(fabs(aJets[ijet].eta())<2.5) 
		{
		  nABtags[0] += (aJets[ijet].btag1>2.0);
		  nABtags[1] += (aJets[ijet].btag2>0.244);
		  nABtags[2] += (aJets[ijet].btag3>0.275);
		}
	      if(aJets[ijet].pt()<30) continue;  // ???????????????????????????
	      nAJetsLoose      += hasObjectId(aJets[ijet].pid,JETID_LOOSE);
	      nAJetsTight      += hasObjectId(aJets[ijet].pid,JETID_TIGHT);
	      nAJetsPUIdLoose  += hasObjectId(aJets[ijet].pid,JETID_OPT_LOOSE);
	      nAJetsPUIdMedium += hasObjectId(aJets[ijet].pid,JETID_OPT_MEDIUM);
	    }
	}
      passJetveto=(nAJetsGood30==0); //(nAJetsLoose==0);
      passBveto=(nABtags[1]==0); 
      passDphijmet=(mindphijmet>0.5);
      if(nAJetsGood30==0) passDphijmet=(mindphijmet15>0.5);
      passBalanceCut=(zvvs[0].pt()/zll.pt()>0.4 && zvvs[0].pt()/zll.pt()<1.8);

      //CHS jets
      int nGoodCHSJets(0);
      PhysicsObjectJetCollection chsJets= variedCHSJets[0];
      LorentzVector chsClusteredMetP4(zll); chsClusteredMetP4 *= -1;
      float mindphichsjmet(999999.),mindphichsjmet15(999999.);
      for(size_t ijet=0; ijet< chsJets.size(); ijet++) 
	{
	  float idphichsjmet( fabs(deltaPhi(chsJets[ijet].phi(),CHSzvvs[0].phi()) ) );
	  if(chsJets[ijet].pt()>15) if(idphichsjmet<mindphichsjmet15)  mindphichsjmet15=idphichsjmet;
	  if(chsJets[ijet].pt()>30) if(idphichsjmet<mindphichsjmet)    mindphichsjmet=idphichsjmet;
	  bool isGoodJet   = hasObjectId(chsJets[ijet].pid,JETID_CUTBASED_LOOSE);
	  if(!isGoodJet) continue;
	  chsClusteredMetP4 -= chsJets[ijet];	  
	  if(chsJets[ijet].pt()>30) nGoodCHSJets++;
	}
      bool passDphichsjmet=(mindphichsjmet>0.5);
      if(nGoodCHSJets==0) passDphichsjmet=(mindphichsjmet15>0.5);

      //ad-hoc cut for obvious correlations between MET and a lepton
      double dphil1met=fabs(deltaPhi(lep1.phi(),zvvs[0].phi()));
      double dphil2met=fabs(deltaPhi(lep2.phi(),zvvs[0].phi()));
      bool passLMetVeto(true);
      if(!use2011Id && zvvs[0].pt()>60 && min(dphil1met,dphil2met)<0.2) passLMetVeto=false;
      
      //other mets
      METUtils::stRedMET aRedMetOut,chsRedMetOut; 
      LorentzVector aRedMet=METUtils::redMET(METUtils::INDEPENDENTLYMINIMIZED, lep1, 0, lep2, 0, aClusteredMetP4, zvvs[0],false,&aRedMetOut);
      double aRedMetL=aRedMetOut.redMET_l;
      double aRedMetT=aRedMetOut.redMET_t;
      LorentzVector chsRedMet=METUtils::redMET(METUtils::INDEPENDENTLYMINIMIZED, lep1, 0, lep2, 0, chsClusteredMetP4, CHSzvvs[0],false,&chsRedMetOut);
      LorentzVector assocMetP4(phys.met[1]);
      LorentzVector min3Met( min(zvvs[0], min(assocMetP4,aClusteredMetP4)) );
      passRedMet=(aRedMet.pt()>60); if(!use2011Id) passRedMet=(aRedMet.pt()>70);

      //transverse masses
      double aMT=METUtils::transverseMass(zll,zvvs[0],true);
      double atypeIMT=METUtils::transverseMass(zll,fullTypeIMetP4,true);

      //
      //RUN PRESELECTION AND CONTROL PLOTS
      //
      if(isMC && use2011Id) weight *= llScaleFactor*llTriggerEfficiency;
      if(hasTrigger)   {mon.fillHisto("eventflow",tags_full,0,weight); mon.fillHisto("eventflow_MVA",tags_full,0,weight);}
      if(passId)       {mon.fillHisto("eventflow",tags_full,1,weight); mon.fillHisto("eventflow_MVA",tags_full,1,weight);}
      if(passIdAndIso) {mon.fillHisto("eventflow",tags_full,2,weight); mon.fillHisto("eventflow_MVA",tags_full,2,weight);}
      else continue;

      // Control Regions
      bool WZ_ctrl_pt ( passZpt && nextraleptons==1 && passZmass && passBveto );
      bool WZ_ctrl_m  ( passZpt && nextraleptons==1 && passBveto );
      bool T_ctrl     ( passZpt && nABtags>0 && isZsideBand );
      bool T_ctrl_side( passZpt && nABtags>0 );
      bool WW_ctrl    ( passZpt && passBveto && (nAJetsLoose==0) && passBalanceCut && passDphijmet && pass3dLeptonVeto && isZsideBand );
      bool WW_ctrl_1  ( passZpt && isZsideBand );
      bool WW_ctrl_2  ( passZpt && isZsideBand && passBveto );
      bool WW_ctrl_3  ( passZpt && isZsideBand && passBveto && passBalanceCut && (nAJetsLoose==0) );
      bool WW_ctrl_4  ( passZpt && isZsideBand && passBveto && passBalanceCut && (nAJetsLoose==0) && passDphijmet);
      bool WW_ctrl_5  ( passZpt && isZsideBand && passBveto && passBalanceCut && (nAJetsLoose==0) && passDphijmet && pass3dLeptonVeto);
      bool WW_ctrl_m  ( passZpt && passRedMet  && passBveto && passBalanceCut && (nAJetsLoose==0) && passDphijmet && pass3dLeptonVeto);

      if(WZ_ctrl_pt)  mon.fillHisto("Ctrl_WZ_RedMet",tags_full,aRedMet.pt(),weight);
      if(WZ_ctrl_m){
	mon.fillHisto("Ctrl_WZ_zmass",tags_full,zll.mass(),weight);
	if(passZmass) mon.fillHisto("Ctrl_WZ_Mt", tags_full, sqrt((2*zvvs[0].pt()*extraLeptonsP4[0].pt())*(1-cos(deltaPhi(zvvs[0].phi(),extraLeptonsP4[0].phi())))), weight);
      }
      if(T_ctrl)      mon.fillHisto("Ctrl_T_zmass",tags_full,zll.mass(),weight);
      if(T_ctrl_side) mon.fillHisto("Ctrl_Tside_RedMet",tags_full,aRedMet.pt(),weight);
      if(WW_ctrl)     mon.fillHisto("Ctrl_WW_RedMet",tags_full,aRedMet.pt(),weight);
      if(WW_ctrl_1)   mon.fillHisto("Ctrl_WW_RedMet1",tags_full,aRedMet.pt(),weight);
      if(WW_ctrl_2)   mon.fillHisto("Ctrl_WW_RedMet2",tags_full,aRedMet.pt(),weight);
      if(WW_ctrl_3)   mon.fillHisto("Ctrl_WW_RedMet3",tags_full,aRedMet.pt(),weight);
      if(WW_ctrl_4)   mon.fillHisto("Ctrl_WW_RedMet4",tags_full,aRedMet.pt(),weight);
      if(WW_ctrl_5)   mon.fillHisto("Ctrl_WW_RedMet5",tags_full,aRedMet.pt(),weight);
      if(WW_ctrl_m)   mon.fillHisto("Ctrl_WW_zmass",tags_full,zll.mass(),weight);

      //Sherpa control 
      mon.fillHisto("njets_pres", tags_full, nAJetsLoose,weight);

      //
      // EVENT SELECTION
      //
      mon.fillHisto("zmass",       tags_full, zll.mass(), weight);  
      if(passZmass){
	mon.fillHisto("eventflow",   tags_full, 3,            weight); mon.fillHisto("eventflow_MVA",tags_full,3,weight);
	mon.fillHisto("zpt"      ,   tags_full, zll.pt(),     weight);      
	if(nAJetsLoose==0) mon.fillHisto("zpt_noJet", tags_full, zll.pt(), weight);      
	if(nAJetsLoose==0 && genmatchid[0]>-1 && genmatchid[1]>-1 && genmatchid[0]!=genmatchid[1]) mon.fillHisto("zptGen_noJet", tags_full, genZP4.pt(), weight);      
	mon.fillHisto("zeta"     ,   tags_full, zll.eta(),    weight);
	mon.fillHisto("nvtx"     ,   tags_full, ev.nvtx,      weight);
	mon.fillHisto("nvtxraw"  ,   tags_full, ev.nvtx,      1);
	mon.fillHisto("rho"      ,   tags_full, ev.rho,       weight);
	mon.fillHisto("rho25"    ,   tags_full, ev.rho25Neut, weight);
	
	  if(passZpt){
	      mon.fillHisto("eventflow",tags_full,4,weight); mon.fillHisto("eventflow_MVA",tags_full,4,weight);
              //mon.fillHisto("pfvbfcount_total",tags_cat,ev.ngenITpu,weight);
	      
	      //analyze lepton kinematics
	      LorentzVector leadingLep(phys.leptons[0].pt()>phys.leptons[1].pt() ? phys.leptons[0]: phys.leptons[1]);
	      LorentzVector trailerLep(phys.leptons[0].pt()>phys.leptons[1].pt() ? phys.leptons[1]: phys.leptons[0]);
	      mon.fillHisto("leadeta"     ,   tags_full, leadingLep.eta()   ,weight);
	      mon.fillHisto("leadpt"      ,   tags_full, leadingLep.pt()    ,weight);
	      mon.fillHisto("trailereta"  ,   tags_full, trailerLep.eta()   ,weight);
	      mon.fillHisto("trailerpt"   ,   tags_full, trailerLep.pt()    ,weight);
	      mon.fillHisto("nleptons",tags_full,2+nextraleptons,weight);
	      for(size_t iel=0; iel<extraLeptonsP4.size(); iel++)
		{
		  mon.fillHisto("thirdleptonpt" ,   tags_full,extraLeptonsP4[iel].pt()     ,weight);
		  mon.fillHisto("thirdleptoneta",   tags_full,extraLeptonsP4[iel].eta()   ,weight);
		}

	      if(pass3dLeptonVeto){
		  mon.fillHisto("eventflow",tags_full,5,weight); mon.fillHisto("eventflow_MVA",tags_full,5,weight);

		  //pre-tagged jet control
		  for(size_t ij=0; ij<aGoodIdJets.size(); ij++)
		    {
		      mon.fillHisto("pfjetpt",  tags_full, aGoodIdJets[ij].pt(),weight);
		      mon.fillHisto("pfjeteta",  tags_full, fabs(aGoodIdJets[ij].eta()),weight);
		      //mon.fillHisto("pfjetpt",  tags_small, aGoodIdJets[ij].pt(),weight);
		      //mon.fillHisto("pfjeteta",  tags_small, fabs(aGoodIdJets[ij].eta()),weight);
		    }
		  
		  //final jet control
		  mon.fillHisto("npfjets",              tags_full, nAJetsLoose,weight);
		  mon.fillHisto("npfjetsvspu",          tags_small, ev.ngenITpu, nAJetsLoose,weight);
		  mon.fillHisto("npfjetstightvspu",     tags_small, ev.ngenITpu, nAJetsTight,weight);
		  mon.fillHisto("npfjetspuidloosevspu", tags_small, ev.ngenITpu, nAJetsPUIdLoose,weight);
		  mon.fillHisto("npfjetspuidmediumvspu",tags_small, ev.ngenITpu, nAJetsPUIdMedium,weight);
		      
		  //sub-divide for good jets
		  int eventSubCat  = eventCategoryInst.Get(phys,&aGoodIdJets);
		  TString tag_subcat = eventCategoryInst.GetLabel(eventSubCat);
		  /// Daniele's personal selection (NO BINS WITH > 0 JETS!!!)
		  if( !(tag_subcat.Contains("eq1jets") || tag_subcat.Contains("eq2jets")) ) {
		    tags_full.push_back(tag_cat+tag_subcat);
		    //if(tag_subcat=="eq1jets" || tag_subcat=="geq2jets")tags_full.push_back(tag_cat + "geq1jets");
		    if(tag_cat=="mumu" || tag_cat=="ee"){tags_full.push_back(string("ll")+tag_subcat);  if(tag_subcat=="eq1jets" || tag_subcat=="geq2jets")tags_full.push_back(string("ll")+string("geq1jets"));   }
		  } // end Daniele's personal selection

// 		  if(tag_subcat=="vbf"){
// 		    TString tag_subcatVBF = tag_subcat;
// 		    if(fabs(aGoodIdJets[0].eta())<2.1 && fabs(aGoodIdJets[1].eta())<2.1){tag_subcatVBF+="2";}else
// 		      if(fabs(aGoodIdJets[0].eta())<2.1 || fabs(aGoodIdJets[1].eta())<2.1){tag_subcatVBF+="1";}else
//                                                                                               {tag_subcatVBF+="0";}
// 		    tags_full.push_back(tag_cat+tag_subcatVBF);
// 		    if(tag_cat=="mumu" || tag_cat=="ee"){tags_full.push_back(string("ll")+tag_subcatVBF); }
// 		  }
		  //if(tag_subcat!="vbf") tags_full.push_back(tag_cat + "novbf");
		  //if(tag_subcat=="geq2jets" || tag_subcat=="vbf")tags_full.push_back(tag_cat + "geq2jetsInc");
		  //if(tag_cat=="mumu" || tag_cat=="ee")tags_full.push_back(string("ll")+tag_subcat);
		      
		  // jet veto
		  if(passJetveto) {
		    mon.fillHisto("eventflow",tags_full,6,weight); mon.fillHisto("eventflow_MVA",tags_full,6,weight);

		  //pre-tagged jet control
		  for(size_t ij=0; ij<aGoodIdJets.size(); ij++)
		    {
		      if(aGoodIdJets[ij].pt()<30 || fabs(aGoodIdJets[ij].eta())>2.5) continue;
			Float_t ijetbtags[]={aGoodIdJets[ij].btag1, aGoodIdJets[ij].btag2, aGoodIdJets[ij].btag3};
			for(size_t ibtag=0; ibtag<3; ibtag++)
			  {
			    if(isMC)
			      {
				if(fabs(aGoodIdJets[ij].flavid)==5) mon.fillHisto(btagAlgos[ibtag]+"bpfjetstags",     tags_small, ijetbtags[ibtag], weight);
				else                                mon.fillHisto(btagAlgos[ibtag]+"otherpfjetstags", tags_small, ijetbtags[ibtag], weight);
			      }
			    mon.fillHisto(btagAlgos[ibtag]+"pfjetstags",  tags_small, ijetbtags[ibtag],weight);
			  }
		    }
		  for(size_t ibtag=0; ibtag<3; ibtag++) mon.fillHisto("npfjetsbtags"+btagAlgos[ibtag],  tags_full, nABtags[ibtag] ,weight);
		  
		  //b-veto
		  if(passBveto)
		    {
		      mon.fillHisto("eventflow",tags_full,/*6*/ 7,weight); mon.fillHisto("eventflow_MVA",tags_full,/*6*/ 7,weight);

		      if(zvvs[0].pt()>0)  mon.fillHisto("mindphijmet_0",  tags_full, nAJetsGood30==0 ? mindphijmet15:mindphijmet,weight);
		      if(zvvs[0].pt()>25) mon.fillHisto("mindphijmet_25", tags_full, nAJetsGood30==0 ? mindphijmet15:mindphijmet,weight);
		      if(zvvs[0].pt()>50) mon.fillHisto("mindphijmet_50", tags_full, nAJetsGood30==0 ? mindphijmet15:mindphijmet,weight);
		      if(zvvs[0].pt()>50) mon.fillHisto("mindphijmet",    tags_full, nAJetsGood30==0 ? mindphijmet15:mindphijmet,weight);

		      if(passDphijmet) 
			{
			  mon.fillHisto("eventflow",tags_full,/*7*/ 8,weight); mon.fillHisto("eventflow_MVA",tags_full,/*7*/ 8,weight);
			  
			  //VBF monitoring
			  float dphijj(-1),hardpt(-1);
// 			  if(aGoodIdJets.size()>=2)
// 			  {
// 			      LorentzVector vbfSyst=aGoodIdJets[0]+aGoodIdJets[1];
// 			      LorentzVector hardSyst=vbfSyst+zvvs[0]+zll;
// 			      hardpt=hardSyst.pt();
// 			      dphijj=deltaPhi(aGoodIdJets[0].phi(),aGoodIdJets[1].phi());
// 			      double maxEta=max(fabs(aGoodIdJets[0].eta()),fabs(aGoodIdJets[1].eta()));
// 			      double minEta=min(fabs(aGoodIdJets[0].eta()),fabs(aGoodIdJets[1].eta()));
// 			      float detajj=maxEta-minEta;
// 			      mon.fillHisto("pfvbfcandjetpt",     tags_cat, fabs(aGoodIdJets[0].pt()),weight);
// 			      mon.fillHisto("pfvbfcandjetpt",     tags_cat, fabs(aGoodIdJets[1].pt()),weight);
//                               if(aGoodIdJets[0].pt()>30 && aGoodIdJets[1].pt()>30){
//                                  mon.fillHisto("pfvbfcandjeteta",     tags_cat, fabs(maxEta),weight);
//                                  mon.fillHisto("pfvbfcandjeteta",     tags_cat, fabs(minEta),weight);
//                                  mon.fillHisto("pfvbfcandjetdeta",     tags_cat, fabs(detajj),weight);
// 				 mon.fillHisto("pfvbfpremjj",     tags_cat, vbfSyst.mass(),weight);
//                                  if(fabs(detajj)>4.0)//4.5
//                                    {
//                                      mon.fillHisto("pfvbfmjj",         tags_cat, vbfSyst.mass(), weight);
//                                      mon.fillHisto("pfvbfmjjvsdeta",   tags_cat, vbfSyst.mass(), fabs(detajj),weight);
//                                      mon.fillHisto("pfvbfmjjvshardpt", tags_cat, vbfSyst.mass(), hardpt,weight);
//                                      if(vbfSyst.mass()>500)//500 
//                                        {
//                                          mon.fillHisto("pfvbfhardpt",     tags_cat, hardpt,weight);
//                                          int ncjv(0);
//                                          float htcjv(0);
//                                          for(size_t iotherjet=2; iotherjet<aGoodIdJets.size(); iotherjet++)
//                                            {
//                                              if(aGoodIdJets[iotherjet].pt()<30 || aGoodIdJets[iotherjet].eta()<minEta || aGoodIdJets[iotherjet].eta()>maxEta) continue;
//                                              htcjv+= aGoodIdJets[iotherjet].pt();
//                                              ncjv++;
//                                            }
//                                          mon.fillHisto("pfvbfcjv",tags_cat,ncjv,weight);
//                                          mon.fillHisto("pfvbfhtcjv",tags_cat,htcjv,weight);

//                                          bool passJetPUId( hasObjectId(aGoodIdJets[0].pid, JETID_OPT_LOOSE) && hasObjectId(aGoodIdJets[1].pid, JETID_OPT_LOOSE)) ;
//                                          mon.fillHisto("pfvbfcount_tagzpt",tags_cat,ev.ngenITpu,weight);
//                                          if(passJetPUId)mon.fillHisto("pfvbfcount_tagzptpuid",tags_cat,ev.ngenITpu,weight);

//                                          if(fabs(minEta)<2.1 && fabs(maxEta)<2.1){mon.fillHisto("pfvbfcount_tagzpttk2",tags_cat,ev.ngenITpu,weight);}else
//                                          if(fabs(minEta)<2.1 || fabs(maxEta)<2.1){mon.fillHisto("pfvbfcount_tagzpttk1",tags_cat,ev.ngenITpu,weight);}else
//                                                                                  {mon.fillHisto("pfvbfcount_tagzpttk0",tags_cat,ev.ngenITpu,weight);}
//                                        }
//                                    }
//                                }
// 			  } // end if(aGoodIdJets.size()>=2)
			  
			  //
			  //SIGNAL REGION NOW : PROCEED WITH CARE
			  //
			  bool hasVbfBlinding(!isMC && runBlinded && tag_subcat=="vbf" && zvvs[0].pt()>70);
			  // se', blinding...
			  //if(runBlinded && (mustBlind || hasVbfBlinding) ) tags_full.clear();  //don't skip the event but don't fill any plot!
			  
			  mon.fillHisto("mindphilmet",tags_full, min(dphil1met,dphil2met) ,weight);
			  mon.fillHisto("maxdphilmet",tags_full, max(dphil1met,dphil2met) ,weight);
			  mon.fillHisto("met_metbeforedphilmet",         tags_full,  zvvs[0].pt(),  weight);
			  mon.fillHisto("met_mindphilmet",tags_full,zvvs[0].pt(),min(dphil1met,dphil2met),weight);
			  mon.fillHisto("deltaleptonpt",tags_full, leadingLep.pt()-trailerLep.pt()    ,weight);
			  mon.fillHisto("deltazpt",tags_full, zll.pt()-zvvs[0].pt(),weight);
			  mon.fillHisto("balance",tags_full, zvvs[0].pt()/zll.pt(),weight);

			  mon.fillHisto("met_met",         tags_full,  zvvs[0].pt(),  weight);
			  mon.fillHisto("met_mvamet",      tags_full,  mvaMetP4.pt(), weight);
			  mon.fillHisto("met_typeImet",    tags_full,  fullTypeIMetP4.pt(),  weight);
			  mon.fillHisto("met_redMet",tags_full,aRedMet.pt(),weight);
			  mon.fillHisto("met_redMetL",tags_full,aRedMetT,weight);
			  mon.fillHisto("met_redMetT",tags_full,aRedMetL,weight);

			  //if(zvvs[0].pt()>70){
			  if(passRedMet){
			    mon.fillHisto("eventflow",tags_full,/*8*/ 9,weight); 
// 			    mon.fillHisto("pfvbfcandjetdphi",     tags_cat, fabs(dphijj),weight);
// 			    mon.fillHisto("pfvbfhardptfinal",     tags_cat, hardpt,weight); 

			    // Final distributions
			    mon.fillHisto("mt_final",          tags_full, aMT,          weight);
			    mon.fillHisto("zpt_final",         tags_full, zll.pt(),     weight);
			    mon.fillHisto("met_met_final",     tags_full, zvvs[0].pt(), weight);
			    mon.fillHisto("met_redMet_final",  tags_full, aRedMet.pt(), weight);
			    mon.fillHisto("met_redMetL_final", tags_full, aRedMetL,     weight);

			    if(passBalanceCut) {
			      mon.fillHisto("eventflow",tags_full,10,weight); mon.fillHisto("eventflow_MVA",tags_full,9,weight);
			    }//end passBalanceCut
			  } // end passRedMet

			  if(zvvs[7].pt()>70) { // MVA MET
			    mon.fillHisto("eventflow_MVA",tags_full,/*8*/ 9,weight);
			    if(passBalanceCut) {
			      mon.fillHisto("eventflow_MVA",tags_full,10,weight); mon.fillHisto("eventflow_MVA",tags_full,9,weight);
			    }//end passBalanceCut
			  }//end MVA MET

// 			  if(passLMetVeto)
// 			    {
// 			      mon.fillHisto("met_met",         tags_full,  zvvs[0].pt(),  weight);
// 			      mon.fillHisto("met_mvamet",      tags_full,  mvaMetP4.pt(), weight);
// 			      mon.fillHisto("met_typeImet",    tags_full,  fullTypeIMetP4.pt(),  weight);
// 			      mon.fillHisto("met_redMet",tags_full,aRedMet.pt(),weight);
// 			      mon.fillHisto("met_redMetL",tags_full,aRedMetT,weight);
// 			      mon.fillHisto("met_redMetT",tags_full,aRedMetL,weight);
// 			      if( (mustBlind || hasVbfBlinding || isMC_GG || isMC_VBF) ) {
// 				mon.fillHisto("met_met_blind",tags_full,zvvs[0].pt(),weight);
// 				mon.fillHisto("met_mvamet_blind",      tags_full,  mvaMetP4.pt(), weight);
// 				mon.fillHisto("met_typeImet_blind",    tags_full,  fullTypeIMetP4.pt(),  weight);
// 				mon.fillHisto("met_redMet_blind",tags_full,aRedMet.pt(),weight);
// 				mon.fillHisto("mt_blind",tags_full,aMT,weight);
// 			      }
			      
// 			      mon.fillProfile("metvsrun",          tags_full, ev.run,            zvvs[0].pt(), weight);
// 			      mon.fillProfile("metvsavginstlumi",  tags_full, ev.curAvgInstLumi, zvvs[0].pt(), weight);
// 			      mon.fillProfile("nvtxvsrun",         tags_full, ev.run,            ev.nvtx,      weight);
// 			      mon.fillProfile("nvtxvsavginstlumi", tags_full, ev.curAvgInstLumi, ev.nvtx,      weight);
			      
// 			      mon.fillHisto("met_met_vspu",         tags_full,ev.ngenITpu,zvvs[0].pt(),       weight);
// 			      mon.fillHisto("met_mvamet_vspu",      tags_full,ev.ngenITpu,mvaMetP4.pt(),      weight);
// 			      mon.fillHisto("met_typeImet_vspu",    tags_full,ev.ngenITpu,fullTypeIMetP4.pt(),    weight);
// 			      mon.fillHisto("met_redMet_vspu",      tags_full,ev.ngenITpu,aRedMet.pt(),weight);
// 			      mon.fillHisto("met_redMetCHS_vspu",   tags_full,ev.ngenITpu,chsRedMet.pt(),weight);

// 			      mon.fillHisto("metvsmt",tags_full,zvvs[0].pt(),aMT,weight);
// 			      mon.fillHisto("typeImetvstypeImt",tags_full,fullTypeIMetP4.pt(),atypeIMT,weight);			      

// 			      mon.fillHisto("mt",tags_full,aMT,weight);
// 			      mon.fillHisto("typeImt",tags_full,atypeIMT,weight);
// 			      mon.fillHisto("mt_raw",tags_full,aMT,noLShapeWeight);
			      
// 			      mon.fillHisto("RunDep_Yields",tags_full,ev.run,weight);
// 			      mon.fillProfile("RunDep_Met"   ,tags_full,ev.run, zvvs[0].pt(),weight);
// 			    }//end passLMetVeto
			}//end passDphijmet
		    }//end passBveto
		  }//end passJetveto
	      }else{ //end pass3rdLeptonVeto
		if(nextraleptons==1 && passBveto && passDphijmet && passLMetVeto) mon.fillHisto("met_met3leptons",tags_full,zvvs[0].pt(),weight);
	      }
	  }//end passZpt
      }//end passZmass

      if(passZmass && passZpt /*&& pass3dLeptonVeto*/ && passJetveto && passBveto && passDphijmet && passBalanceCut && passRedMet)
	mon.fillHisto("nleptons_final",tags_full,2+nextraleptons,weight);

      //
      // N-1 plots
      //
      /*
      if(passZmass && passZpt && pass3dLeptonVeto && passBveto && (passDphijmet && passLMetVeto) && zvvs[0].pt()>70) { mon.fillHisto("mtNM1", tags_full, aMT ,weight); }
      if(             passZpt && pass3dLeptonVeto && passBveto && (passDphijmet && passLMetVeto) && zvvs[0].pt()>70) { mon.fillHisto("zmassNM1",tags_full,zll.mass(),weight); }
      if(passZmass            && pass3dLeptonVeto && passBveto && (passDphijmet && passLMetVeto) && zvvs[0].pt()>70) { mon.fillHisto("zptNM1",tags_full,zll.pt(),weight); }
      if(passZmass && passZpt                     && passBveto && (passDphijmet && passLMetVeto) && zvvs[0].pt()>70) { mon.fillHisto("nleptonsNM1",tags_full,2+nextraleptons,weight); }
      if(passZmass && passZpt && pass3dLeptonVeto              && (passDphijmet && passLMetVeto) && zvvs[0].pt()>70) { mon.fillHisto("npfjetsbtagsJPNM1", tags_full, nABtags[2] ,weight); }
      if(passZmass && passZpt && pass3dLeptonVeto && passBveto                                   && zvvs[0].pt()>70) { 
	mon.fillHisto("mindphijmetNM1", tags_full, nAJetsGood30==0 ? mindphijmet15:mindphijmet ,weight); 
	mon.fillHisto("mindphichsjmetNM1", tags_full, nGoodCHSJets==0 ? mindphichsjmet15:mindphichsjmet ,weight); 
      }
      if(passZmass && passZpt && pass3dLeptonVeto && passBveto && (passDphijmet && passLMetVeto)                   ) { 
	mon.fillHisto("met_metNM1", tags_full, zvvs[0].pt() ,weight); 
	mon.fillHisto("met_redMetNM1", tags_full, aRedMet.pt(),weight); 
      }
      if(passZmass && passZpt && pass3dLeptonVeto && passBveto && (passDphichsjmet && passLMetVeto)                   ) { 
	mon.fillHisto("met_redMetCHSNM1", tags_full, chsRedMet.pt(),weight); 
      }
      */

      if(passZmass && passZpt && pass3dLeptonVeto && passJetveto && passBveto && passDphijmet && passBalanceCut && passRedMet) { mon.fillHisto("mtNM1", tags_full, aMT ,weight); }
      if(             passZpt && pass3dLeptonVeto && passJetveto && passBveto && passDphijmet && passBalanceCut && passRedMet) { mon.fillHisto("zmassNM1",tags_full,zll.mass(),weight); }
      if(passZmass            && pass3dLeptonVeto && passJetveto && passBveto && passDphijmet && passBalanceCut && passRedMet) { mon.fillHisto("zptNM1",tags_full,zll.pt(),weight); }
      if(passZmass && passZpt                     && passJetveto && passBveto && passDphijmet && passBalanceCut && passRedMet) { mon.fillHisto("nleptonsNM1",tags_full,2+nextraleptons,weight); }
      if(passZmass && passZpt && pass3dLeptonVeto                && passBveto && passDphijmet && passBalanceCut && passRedMet) { mon.fillHisto("npfjetsNM1", tags_full,nAJetsLoose,weight); }
      if(passZmass && passZpt && pass3dLeptonVeto && passJetveto              && passDphijmet && passBalanceCut && passRedMet) { mon.fillHisto("npfjetsbtagsJPNM1", tags_full, nABtags[2] ,weight); }
      if(passZmass && passZpt && pass3dLeptonVeto && passJetveto && passBveto                 && passBalanceCut && passRedMet) { 
	mon.fillHisto("mindphijmetNM1", tags_full, nAJetsGood30==0 ? mindphijmet15:mindphijmet ,weight); 
	mon.fillHisto("mindphichsjmetNM1", tags_full, nGoodCHSJets==0 ? mindphichsjmet15:mindphichsjmet ,weight); 
      }
      if(passZmass && passZpt && pass3dLeptonVeto && passJetveto && passBveto && passDphijmet                   && passRedMet) { mon.fillHisto("balanceNM1",tags_full,zvvs[0].pt()/zll.pt(),weight); }
      if(passZmass && passZpt && pass3dLeptonVeto && passJetveto && passBveto && passDphijmet && passBalanceCut              ) { 
	mon.fillHisto("met_metNM1", tags_full, zvvs[0].pt() ,weight); 
	mon.fillHisto("met_redMetNM1", tags_full, aRedMet.pt(),weight); 
      }
      if(passZmass && passZpt && pass3dLeptonVeto && passJetveto && passBveto && passDphichsjmet && passBalanceCut           ) { 
	mon.fillHisto("met_redMetCHSNM1", tags_full, chsRedMet.pt(),weight); 
      }


      //MET control in the sideband
      bool passSB( ((zll.mass()>40 && zll.mass()<70) || (zll.mass()>110 && zll.mass()<200)) && zll.pt()>55 );      
      if(passSB && pass3dLeptonVeto && passDphijmet && !passBveto && passLMetVeto) mon.fillHisto("met_metSB",tags_full,zvvs[0].pt(),weight);
     
      //
      // HISTOS FOR STATISTICAL ANALYSIS (include systematic variations)
      //
      //Fill histogram for posterior optimization, or for control regions
      for(size_t ivar=0; ivar<nvarsToInclude; ivar++){
        float iweight = weight;                                               //nominal
        if(ivar==9)                         iweight *=TotalWeight_plus;        //pu up
        if(ivar==10)                        iweight *=TotalWeight_minus;       //pu down
        if(ivar<=14 && ivar>=11 && isMC_GG) iweight *=ev.hptWeights[ivar-10]/ev.hptWeights[0];   //ren/fact     scales   
	if((ivar==17 || ivar==18) && isMC_GG) iweight *= (lShapeWeights[0]!=0?lShapeWeights[ivar-16]/lShapeWeights[0] : 1.0); //lineshape weights

	float Sherpa_weight(1.);
	if(use2011Id && (ivar==19 || ivar==20)){
	  if( zll.pt() < 84) Sherpa_weight = 0.976138;
	  else if( zll.pt()< 98) Sherpa_weight = 1.01469;
	  else if( zll.pt()< 112) Sherpa_weight = 1.00612;
	  else if( zll.pt()< 126) Sherpa_weight = 0.981601;
	  else if( zll.pt()< 140) Sherpa_weight = 0.969641;
	  else if( zll.pt()< 154) Sherpa_weight = 0.980155;
	  else if( zll.pt()< 168) Sherpa_weight = 1.00758;
	  else if( zll.pt()< 182) Sherpa_weight = 1.04195;
	  else if( zll.pt()< 196) Sherpa_weight = 1.07586;
	  else if( zll.pt()< 210) Sherpa_weight = 1.10551;
	  else if( zll.pt()< 224) Sherpa_weight = 1.12925;
	  else if( zll.pt()< 238) Sherpa_weight = 1.14636;
	  else if( zll.pt()< 252) Sherpa_weight = 1.15645;
	  else if( zll.pt()< 266) Sherpa_weight = 1.15935;
	  else if( zll.pt()< 280) Sherpa_weight = 1.15498;
	  else if( zll.pt()< 294) Sherpa_weight = 1.14343;
	  else if( zll.pt()< 308) Sherpa_weight = 1.12491;
	  else if( zll.pt()< 322) Sherpa_weight = 1.09977;
	  else if( zll.pt()< 336) Sherpa_weight = 1.06847;
	  else                    Sherpa_weight = 1.03156;
	  if(ivar==20) Sherpa_weight = (1./Sherpa_weight);
	}
       if(!use2011Id && (ivar==13 || ivar==14)){
         if( zll.pt() < 84) Sherpa_weight = 1.0724;
         else if( zll.pt()< 112) weight = 0.981944;
         else if( zll.pt()< 126) weight = 0.945408;   
         else if( zll.pt()< 140) weight = 0.947075;
         else if( zll.pt()< 154) weight = 0.974582;
         else if( zll.pt()< 168) weight = 1.00435;
         else if( zll.pt()< 182) weight = 1.01752;
         else if( zll.pt()< 196) weight = 1.00596;
         else if( zll.pt()< 210) weight = 0.970417;
         else if( zll.pt()< 224) weight = 0.915903;
         else if( zll.pt()< 238) weight = 0.848171;
         else if( zll.pt()< 252) weight = 0.772162;
         else if( zll.pt()< 266) weight = 0.691807;
         else if( zll.pt()< 280) weight = 0.610271;
         else if( zll.pt()< 294) weight = 0.530154;
         else if( zll.pt()< 308) weight = 0.453577;
         else if( zll.pt()< 322) weight = 0.382189;
         else if( zll.pt()< 336) weight = 0.317164;                                     
         else if( zll.pt()< 350) weight = 0.25922; 
       }

	//recompute MET/MT if JES/JER was varied
	LorentzVector zvv = zvvs[ivar>8 ? 0 : ivar];
	//PhysicsObjectJetCollection &varJets=variedAJets[ivar>4 ? 0  : ivar];
	//PhysicsObjectJetCollection &varJets = ( ivar>4 && useJERsmearing==false ? recoJets : variedAJets[ivar>4 ? 0  : ivar] );
	PhysicsObjectJetCollection &varJets = ( ivar<=4 ? variedAJets[ivar] : ( useJERsmearing ? variedAJets[0] : recoJets ) );
	PhysicsObjectJetCollection tightVarJets;
	LorentzVector clusteredMetP4(zll); clusteredMetP4 *= -1;
	bool passLocalJetveto(true);
	bool passLocalBveto(true); //bool passLocalBveto(passBveto);
	bool passLocalDphijmet(true);
	bool passLocalBalanceCut(true);
	float localmindphijmet(999999.),localmindphijmet15(999999.);
	int localNAJetsGood30(0);
	for(size_t ijet=0; ijet<varJets.size(); ijet++){
	  //if(!hasObjectId(varJets[ijet].pid,JETID_LOOSE)) continue;

	  // dphi
	  float idphijmet( fabs(deltaPhi(varJets[ijet].phi(),zvv.phi()) ) );
	  if(varJets[ijet].pt()>15) if(idphijmet<localmindphijmet15) localmindphijmet15 = idphijmet;
	  if(varJets[ijet].pt()>30) if(idphijmet<localmindphijmet)   localmindphijmet   = idphijmet;

	  bool isGoodJet = hasObjectId(aJets[ijet].pid,JETID_LOOSE); //JETID_TIGHT);
	  if(usePUsubJetId) isGoodJet = hasObjectId(aJets[ijet].pid,JETID_CUTBASED_LOOSE); 
	  if(isGoodJet) {
	    clusteredMetP4 -= varJets[ijet];

	    if(useJetsOnlyInTracker && fabs(varJets[ijet].eta())>2.5) continue; 
	    tightVarJets.push_back( varJets[ijet] );
	    if(varJets[ijet].pt()>30)localNAJetsGood30++;

	    //if(aJets[ijet].pt()<30) continue; // ???????????????????????????
	    if(varJets[ijet].pt()<20) continue; 
	    if(fabs(varJets[ijet].eta())<2.5) {
	      // b-tag
	      if(ivar==15)      passLocalBveto &= (varJets[ijet].btag2<0.250); 
	      else if(ivar==16) passLocalBveto &= (varJets[ijet].btag2<0.240); 
	      else              passLocalBveto &= (varJets[ijet].btag2<0.244); 
	    }
	  }
	}
	passLocalJetveto=(localNAJetsGood30==0); 
	passLocalDphijmet=(localmindphijmet>0.5);
	if(localNAJetsGood30==0) passLocalDphijmet=(localmindphijmet15>0.5);
	passLocalBalanceCut=(zvv.pt()/zll.pt()>0.4 && zvv.pt()/zll.pt()<1.8);

	float mt = METUtils::transverseMass(zll,zvv,true);
	LorentzVector nullP4(0,0,0,0);
	LorentzVector redMet = METUtils::redMET(METUtils::INDEPENDENTLYMINIMIZED, zll, 0, nullP4, 0, clusteredMetP4, zvv,true);

	// with standard Z mass, Z pt, RedMet (will be variated later)
	bool passPreselection(passZmass && passZpt && pass3dLeptonVeto && passLocalJetveto && passLocalBveto && passLocalDphijmet && passLocalBalanceCut && passRedMet);
	bool passPreselectionMbvetoMzmass(passZpt && pass3dLeptonVeto && passLocalJetveto && passLocalDphijmet && passLocalBalanceCut && passRedMet);          
	
	//re-assign the event category if jets were varied
	int eventSubCat  = eventCategoryInst.Get(phys,&tightVarJets);
	TString tag_subcat = eventCategoryInst.GetLabel(eventSubCat);
	tags_full.clear();
	tags_full.push_back(tag_cat);

	// Daniele's personal selection (NO BINS WITH > 0 JETS!!!)
	if( !(tag_subcat.Contains("eq1jets") || tag_subcat.Contains("eq2jets")) ) {
	  tags_full.push_back(tag_cat+tag_subcat);
	  //if(tag_subcat!="vbf") tags_full.push_back(tag_cat + "novbf");
	  //if(tag_subcat=="eq1jets" || tag_subcat=="geq2jets")tags_full.push_back(tag_cat + "geq1jets");
	  //if(tag_subcat=="geq2jets" || tag_subcat=="vbf")tags_full.push_back(tag_cat + "geq2jetsInc");
	  //if(tag_cat=="mumu" || tag_cat=="ee")tags_full.push_back(string("ll")+tag_subcat);
	  if(tag_cat=="mumu" || tag_cat=="ee"){
	    tags_full.push_back(string("ll")+tag_subcat);  
	    //if(tag_subcat=="eq1jets" || tag_subcat=="geq2jets")tags_full.push_back(string("ll")+string("geq1jets"));   
	  }
// 	  if(tag_subcat=="vbf"){
// 	    TString tag_subcatVBF = tag_subcat;
// 	    if(fabs(tightVarJets[0].eta())<2.1 && fabs(tightVarJets[1].eta())<2.1)      { tag_subcatVBF+="2"; }
// 	    else if(fabs(tightVarJets[0].eta())<2.1 || fabs(tightVarJets[1].eta())<2.1) { tag_subcatVBF+="1"; }
// 	    else                                                                        { tag_subcatVBF+="0"; }
// 	    tags_full.push_back(tag_cat+tag_subcatVBF);
// 	    if(tag_cat=="mumu" || tag_cat=="ee")                                        { tags_full.push_back(string("ll")+tag_subcatVBF); }
// 	  }
	} // end Daniele's personal selection

	if(passPreselection && zvv.pt()>30) mon.fillHisto("mtvar"+varNames[ivar],tags_full,mt,iweight);
	
	/*
	  //DEBUG
	  if(ivar==0 && outTxtFile && tag_subcat=="vbf" && zvv.pt()>70 && passPreselection){
	  fprintf(outTxtFile,"X----------------------------\nCat: %s - %s\n",tag_cat.Data(),tag_subcat.Data());
	  fprintf(outTxtFile,"inputFile = %s\n",url.Data());
	  fprintf(outTxtFile,"run/lumi/event = %i/%i/%i\n",ev.run, ev.lumi, ev.event);
	  fprintf(outTxtFile,"mt = %f met = %f -redMet = %f\n",mt, zvv.pt(), redMet.pt());
	  }
	  
	  if(ivar==0 && outTxtFile && tag_subcat=="geq2jets" && zvv.pt()>100 && mt<250 && passPreselection){
	  fprintf(outTxtFile,"DEBUG----------------------------\nCat: %s - %s\n",tag_cat.Data(),tag_subcat.Data());
	  fprintf(outTxtFile,"subcat = %s inputFile = %s\n",tag_subcat.Data(), url.Data());
	  fprintf(outTxtFile,"run/lumi/event = %i/%i/%i\n",ev.run, ev.lumi, ev.event);
	  fprintf(outTxtFile,"mt = %f met = %f -redMet = %f\n",mt, zvv.pt(), redMet.pt());
	  fprintf(outTxtFile,"nvtx = %i rho=%f rho25 = %f\n",ev.nvtx,ev.rho, ev.rho25Neut);
	  fprintf(outTxtFile,"zll  pt=%f mass=%f eta=%f phi=%f\n",zll.pt(), zll.mass(), zll.eta(), zll.phi());
	  for(unsigned int j=0;j<phys.ajets.size();j++){
	  fprintf(outTxtFile,"jet %i  pt=%f eta=%f phi=%f\n",j, phys.ajets[j].pt(), phys.ajets[j].eta(), phys.ajets[j].phi());
	  }
	  }
	*/
	

	//fill shapes
	for(unsigned int index=0;index<optim_Cuts1_met.size();index++){
	  
	  float minMet=optim_Cuts1_met[index];
	  float minZpt=optim_Cuts1_zpt[index];
	  float deltaZ=optim_Cuts1_zmass[index];

	  bool passLocalRedMet(redMet.pt()>minMet); bool passLocalMet(zvv.pt()>70.);
	  bool passLocalZmass(fabs(zll.mass()-91)<deltaZ);
	  bool passLocalZpt(zll.pt()>minZpt);

	  passPreselection = (passLocalZmass && passLocalZpt && pass3dLeptonVeto && passLocalJetveto && passLocalBveto && passLocalDphijmet && passLocalBalanceCut && passLocalRedMet);
	  passPreselectionMbvetoMzmass = (passLocalZpt && pass3dLeptonVeto && passLocalJetveto && passLocalDphijmet && passLocalBalanceCut && passLocalRedMet); 

	  // Selection with MET (NO RedMET)
	  bool passPreselectionCorrMet(passLocalZmass && passLocalZpt && pass3dLeptonVeto && passLocalJetveto && passLocalBveto && passLocalDphijmet && passLocalBalanceCut && passLocalMet);
	  bool passPreselectionMbvetoMzmassCorrMet(passLocalZpt && pass3dLeptonVeto && passLocalJetveto && passLocalDphijmet && passLocalBalanceCut && passLocalMet); 

	  if( passPreselection ) {
	    mon.fillHisto(TString("mt_redMet_shapes")+varNames[ivar],tags_full,index, mt,iweight);
	    mon.fillHisto(TString("redMet_shapes")+varNames[ivar],tags_full,index, redMet.pt(),iweight);
	    mon.fillHisto(TString("zpt_shapes")+varNames[ivar],tags_full,index, zll.pt(),iweight);
	  }
	  if( passPreselectionMbvetoMzmass && passLocalZmass && passLocalBveto ) {
	    mon.fillHisto("mt_redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,0,iweight);
	    mon.fillHisto("redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,0,iweight);
	    mon.fillHisto("zpt_shapes_NRBctrl"+varNames[ivar],tags_full,index,0,iweight);
	  }
	  if( passPreselectionMbvetoMzmass && isZsideBand && passLocalBveto ) {
	    mon.fillHisto("mt_redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,1,iweight);
	    mon.fillHisto("redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,1,iweight);
	    mon.fillHisto("zpt_shapes_NRBctrl"+varNames[ivar],tags_full,index,1,iweight);
	  }
	  if( passPreselectionMbvetoMzmass && isZsideBandPlus && passLocalBveto ) {
	    mon.fillHisto("mt_redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,2,iweight);
	    mon.fillHisto("redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,2,iweight);
	    mon.fillHisto("zpt_shapes_NRBctrl"+varNames[ivar],tags_full,index,2,iweight);
	  }
	  if( passPreselectionMbvetoMzmass && passLocalZmass && !passLocalBveto ) {
	    mon.fillHisto("mt_redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,3,iweight);
	    mon.fillHisto("redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,3,iweight);
	    mon.fillHisto("zpt_shapes_NRBctrl"+varNames[ivar],tags_full,index,3,iweight);
	  }
	  if( passPreselectionMbvetoMzmass && isZsideBand && !passLocalBveto ) {
	    mon.fillHisto("mt_redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,4,iweight);
	    mon.fillHisto("redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,4,iweight);
	    mon.fillHisto("zpt_shapes_NRBctrl"+varNames[ivar],tags_full,index,4,iweight);
	  }
	  if( passPreselectionMbvetoMzmass && isZsideBandPlus && !passLocalBveto ) {
	    mon.fillHisto("mt_redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,5,iweight);
	    mon.fillHisto("redMet_shapes_NRBctrl"+varNames[ivar],tags_full,index,5,iweight);
	    mon.fillHisto("zpt_shapes_NRBctrl"+varNames[ivar],tags_full,index,5,iweight);
	  }

	  // Mt shapes with MET (NO RedMET) cut
	  if( passPreselectionCorrMet ) {
	    mon.fillHisto(TString("mt_shapes")+varNames[ivar],tags_full,index, mt,iweight);
	  }
	  if( passPreselectionMbvetoMzmassCorrMet && passLocalZmass && passLocalBveto ) {
	    mon.fillHisto("mt_shapes_NRBctrl"+varNames[ivar],tags_full,index,0,iweight);
	  }
	  if( passPreselectionMbvetoMzmassCorrMet && isZsideBand && passLocalBveto ) {
	    mon.fillHisto("mt_shapes_NRBctrl"+varNames[ivar],tags_full,index,1,iweight);
	  }
	  if( passPreselectionMbvetoMzmassCorrMet && isZsideBandPlus && passLocalBveto ) {
	    mon.fillHisto("mt_shapes_NRBctrl"+varNames[ivar],tags_full,index,2,iweight);
	  }
	  if( passPreselectionMbvetoMzmassCorrMet && passLocalZmass && !passLocalBveto ) {
	    mon.fillHisto("mt_shapes_NRBctrl"+varNames[ivar],tags_full,index,3,iweight);
	  }
	  if( passPreselectionMbvetoMzmassCorrMet && isZsideBand && !passLocalBveto ) {
	    mon.fillHisto("mt_shapes_NRBctrl"+varNames[ivar],tags_full,index,4,iweight);
	  }
	  if( passPreselectionMbvetoMzmassCorrMet && isZsideBandPlus && !passLocalBveto ) {
	    mon.fillHisto("mt_shapes_NRBctrl"+varNames[ivar],tags_full,index,5,iweight);
	  }
        }
	
//         for(unsigned int index=0;index<optim_Cuts2_met.size();index++){
// 	  if(varJets.size()>=2 && zvv.pt()>optim_Cuts2_met[index] && varJets[0].pt()>optim_Cuts2_vbfJpt[index] && varJets[1].pt()>optim_Cuts2_vbfJpt[index] &&  fabs(varJets[0].eta()-varJets[1].eta())>optim_Cuts2_vbfdeta[index]  ){
// 	    if(passPreselection                                                         )   mon.fillHisto("VBFmt_shapes"      +varNames[ivar],tags_full,index, (varJets[0] + varJets[1]).M(),iweight);
// 	    if(passPreselectionMbvetoMzmass && passZmass         && passLocalBveto      )   mon.fillHisto("VBFmt_shapes_NRBctrl"+varNames[ivar],tags_full,index,0,iweight);
// 	    if(passPreselectionMbvetoMzmass && isZsideBand       && passLocalBveto      )   mon.fillHisto("VBFmt_shapes_NRBctrl"+varNames[ivar],tags_full,index,1,iweight);
// 	    if(passPreselectionMbvetoMzmass && isZsideBandPlus   && passLocalBveto      )   mon.fillHisto("VBFmt_shapes_NRBctrl"+varNames[ivar],tags_full,index,2,iweight);
// 	    if(passPreselectionMbvetoMzmass && passZmass         && !passLocalBveto     )   mon.fillHisto("VBFmt_shapes_NRBctrl"+varNames[ivar],tags_full,index,3,iweight);
// 	    if(passPreselectionMbvetoMzmass && isZsideBand       && !passLocalBveto     )   mon.fillHisto("VBFmt_shapes_NRBctrl"+varNames[ivar],tags_full,index,4,iweight);
// 	    if(passPreselectionMbvetoMzmass && isZsideBandPlus   && !passLocalBveto     )   mon.fillHisto("VBFmt_shapes_NRBctrl"+varNames[ivar],tags_full,index,5,iweight);
// 	  }
// 	}
      }
  }
  
  printf("\n"); 
  file->Close();
  
  //##############################################
  //########     SAVING HISTO TO FILE     ########
  //##############################################
  //save control plots to file
  outUrl += "/";
  outUrl += outFileUrl + ".root";
  printf("Results save in %s\n", outUrl.Data());

  //save all to the file
  TFile *ofile=TFile::Open(outUrl, "recreate");
  mon.Write();
  ofile->Close();

  if(outTxtFile)fclose(outTxtFile);
}  




