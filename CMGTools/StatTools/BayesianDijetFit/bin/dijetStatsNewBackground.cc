/* -*- mode: c++ -*- */

#include "RooWorkspace.h"
#include "RooHist.h"
#include "RooGenericPdf.h"
#include "RooGaussian.h"
#include "RooAddPdf.h"
#include "RooFitResult.h"
#include "RooDataSet.h"
#include "RooHistPdf.h"
#include "RooProdPdf.h"
#include "RooUniform.h"
#include "RooJeffreysPrior.h"
#include "RooNumIntConfig.h"
#include "RooBinning.h"
#include "RooLognormal.h"
#include "RooAbsPdf.h"
#include "RooAbsReal.h"
#include "RooPlot.h"
#include "RooMinuit.h"

#include "RooStats/BayesianCalculator.h"
#include "RooStats/SimpleInterval.h"
#include "RooStats/ProposalHelper.h"
#include "RooStats/ModelConfig.h"
#include "RooStats/MCMCCalculator.h"
#include "RooStats/MCMCIntervalPlot.h"
#include "RooStats/ProfileLikelihoodCalculator.h"
#include "RooStats/LikelihoodInterval.h"
#include "RooStats/LikelihoodIntervalPlot.h"

#include "Math/Functor.h"
#include "Math/BrentMinimizer1D.h"
#include "RooFunctor.h"
#include "RooFunctor1DBinding.h"

#include "TStopwatch.h"
#include "TFile.h"
#include "TLatex.h"
#include "TAxis.h"
#include "TCanvas.h"
#include "TMath.h"
#include "TROOT.h"
#include "TApplication.h"
#include "TUnixSystem.h"

#include <iostream>
#include <fstream>
#include <string>

#include "StatTools/BayesianDijetFit/interface/DijetHelper.h"
#include "StatTools/BayesianDijetFit/interface/LineShapeDensityPdf.h"
#include "StatTools/BayesianDijetFit/interface/JPMCCalculator.h"
#include "StatTools/BayesianDijetFit/interface/Qstar_qg_3.h"

#include "TMatrix.h"
#include "TVector.h"
#include "TIterator.h"

using namespace RooFit;
using namespace RooStats;
using namespace DijetHelper;

int main(int argc, char* argv[])
{
  bool doAggressiveBkgFit=false;


  vector<double> upper_limits;

  // this is needed to setup the auto-compile for the workspace
  gSystem->SetIncludePath("-I$ROOFITSYS/include");
  // Declare magic numbers here

  double MININVMASS = 890.; // HT-fat-4479/pb
  double MAXINVMASS = 4509.;


  //  std::string DATASETFN="../data/massFat_data_HT_Run2011A_160404_167784_Fat30_ak5.txt";
  //  std::string DATASETFN="../data/massFat_MuPt10_data_HT_Run2011A_160404_167784_Fat30_ak5.txt";

  //  std::string DATASETFN="../data/massFat_MuPt10_data_HT_Run2011AB_160404_180252_FullStat_Fat30_ak5.txt";
  std::string DATASETFN="../data/massFat_data_HT_Run2011AB_160404_180252_FullStat_Fat30_ak5.txt";





  //  double LUMI = 1010.;

  //  double LUMI = 4677.;
  double LUMI = 4974.;

  //  double LUMIERROR=0.045; // relative error on luminosity
  double LUMIERROR=0.022; // relative error on luminosity
  double JES=1.0;        // JES "value"
  double JER=1.0;        // JER "value"
  double JESERROR=0.022;   // relative error on JES
  double JERERROR=0.1;   // relative error on JER
  double NSIGCUTOFF=3.0; // number of +/- "sigma" to cutoff integration of nuisance parameters
  bool USELOGNORM=true;  // use lognormal or gaussian nuisance prior pdfs
  bool USEJEFFREYS=false;  // use jeffreys instead of flat prior
  bool USENEWCOVARIANCE=true;  // Dinkos code for the fit eigenvectors
  bool WRITE=false;

  bool LESS_PARAMETERS=false;

  bool ALLOWSNEGATIVE=true;
  
  // histogram binning (for display only)
  const int NBINS=56;
  Double_t BOUNDARIES[NBINS] = { 220, 244, 270, 296, 325, 354, 386, 419, 453,
				 489, 526, 565, 606, 649, 693, 740, 788, 838,
				 890, 944, 1000, 1058, 1118, 1181, 1246, 1313, 1383,
				 1455, 1530, 1607, 1687, 1770, 1856, 1945, 2037, 2132,
				 2231, 2332, 2438, 2546, 2659, 2775, 2895, 3019, 3147,
	                         3279, 3416, 3558, 3704, 3854, 4010, 4171, 4337, 4509, 4686, 4869};
  
  // start the timer
  TStopwatch t;
  t.Start();


  if(argc<2) {
    std::cout << "Usage: dijetStatsNewBackground signalMass statlevel [numPEs=0]" << std::endl;
    return -1;
  }

  double signalMass = atof(argv[1]);
  int statlevel = atoi(argv[2]);
  int numPEs = argc>=4 ? atoi(argv[3]) : 0;
  const char* cResonance = argc>=5 ? argv[4] : "No Resonance";
  bool verbose_ = argc>=6 ? atoi(argv[5]) : 1;
  std::string sResonance(cResonance);
  int iResonance = 0;

  WRITE = verbose_;

  if (statlevel > 999 && statlevel < 1999) USELOGNORM=false;
  if (statlevel == 2000 || statlevel == 2100) doAggressiveBkgFit=false;


  if (statlevel > 9999)
  {
      USEJEFFREYS=true;
      statlevel=statlevel-10000;
      WRITE = true;
  }

 
  if (sResonance.find("RSGraviton_ak5_fat30") != std::string::npos) iResonance = 10;
  else if (sResonance.find("RSGraviton_ak5_GGtoGG_fat30") != std::string::npos) iResonance = 11;
  else if (sResonance.find("RSGraviton_ak5_QQtoQQ_fat30") != std::string::npos) iResonance = 12;
  else if (sResonance.find("Qstar_ak5_fat30") != std::string::npos) iResonance = 13;
  else if (sResonance.find("RSGraviton_HLT_ak5_GGtoGG_fat30") != std::string::npos) iResonance = 21;
  else if (sResonance.find("RSGraviton_HLT_ak5_QQtoQQ_fat30") != std::string::npos) iResonance = 22;
  else if (sResonance.find("Qstar_HLT_ak5_fat30") != std::string::npos) iResonance = 23;        
                        
  else if (sResonance.find("RSGraviton_HLT_ak5_GGtoGG_pf") != std::string::npos) iResonance = 121;
  else if (sResonance.find("RSGraviton_HLT_ak5_QQtoQQ_pf") != std::string::npos) iResonance = 122;
  else if (sResonance.find("Qstar_HLT_ak5_pf") != std::string::npos) iResonance = 123;   

  else if (sResonance.find("ZprimeToTTbar") != std::string::npos) iResonance = 201;

  else if (sResonance.find("QBH5_ak5_fat") != std::string::npos) iResonance = 1005; 
  else if (sResonance.find("QBH6_ak5_fat") != std::string::npos) iResonance = 1006; 
  else if (sResonance.find("QBH7_ak5_fat") != std::string::npos) iResonance = 1007; 
  else if (sResonance.find("QBH8_ak5_fat") != std::string::npos) iResonance = 1008; 
  else if (sResonance.find("QBH9_ak5_fat") != std::string::npos) iResonance = 1009; 
  else if (sResonance.find("QBH10_ak5_fat") != std::string::npos) iResonance = 1010; 

  else if (sResonance.find("QBH52_ak5_fat") != std::string::npos) iResonance = 1502; 
  else if (sResonance.find("QBH53_ak5_fat") != std::string::npos) iResonance = 1503; 
  else if (sResonance.find("QBH54_ak5_fat") != std::string::npos) iResonance = 1504; 
  else if (sResonance.find("QBH55_ak5_fat") != std::string::npos) iResonance = 1505; 

  else if (sResonance.find("QBH62_ak5_fat") != std::string::npos) iResonance = 1602; 
  else if (sResonance.find("QBH63_ak5_fat") != std::string::npos) iResonance = 1603; 
  else if (sResonance.find("QBH64_ak5_fat") != std::string::npos) iResonance = 1604; 
  else if (sResonance.find("QBH65_ak5_fat") != std::string::npos) iResonance = 1605; 

  else if (sResonance.find("QBH72_ak5_fat") != std::string::npos) iResonance = 1702; 
  else if (sResonance.find("QBH73_ak5_fat") != std::string::npos) iResonance = 1703; 
  else if (sResonance.find("QBH74_ak5_fat") != std::string::npos) iResonance = 1704; 
  else if (sResonance.find("QBH75_ak5_fat") != std::string::npos) iResonance = 1705; 

  else if (sResonance.find("QBH82_ak5_fat") != std::string::npos) iResonance = 1802; 
  else if (sResonance.find("QBH83_ak5_fat") != std::string::npos) iResonance = 1803; 
  else if (sResonance.find("QBH84_ak5_fat") != std::string::npos) iResonance = 1804; 
  else if (sResonance.find("QBH85_ak5_fat") != std::string::npos) iResonance = 1805; 

  else if (sResonance.find("QBH92_ak5_fat") != std::string::npos) iResonance = 1902; 
  else if (sResonance.find("QBH93_ak5_fat") != std::string::npos) iResonance = 1903; 
  else if (sResonance.find("QBH94_ak5_fat") != std::string::npos) iResonance = 1904; 
  else if (sResonance.find("QBH95_ak5_fat") != std::string::npos) iResonance = 1905; 

  else if (sResonance.find("QBH102_ak5_fat") != std::string::npos) iResonance = 1102; 
  else if (sResonance.find("QBH103_ak5_fat") != std::string::npos) iResonance = 1103; 
  else if (sResonance.find("QBH104_ak5_fat") != std::string::npos) iResonance = 1104; 
  else if (sResonance.find("QBH105_ak5_fat") != std::string::npos) iResonance = 1105; 

  else if (sResonance.find("Qstar_qW_ak5_fat") != std::string::npos) iResonance = 2001; 
  else if (sResonance.find("Qstar_qZ_ak5_fat") != std::string::npos) iResonance = 2002; 
  else if (sResonance.find("RSGraviton_WW_ak5_fat") != std::string::npos) iResonance = 2011; 
  else if (sResonance.find("RSGraviton_WZ_ak5_fat") != std::string::npos) iResonance = 2012; 
  else if (sResonance.find("RSGraviton_ZZ_ak5_fat") != std::string::npos) iResonance = 2013; 
  else if (sResonance.find("RSGraviton_ZZ_01_ak5_fat") != std::string::npos) iResonance = 2014; 


  else if (sResonance.find("RSGravitonGG_k0p1_fat30_ak5") != std::string::npos) iResonance = 3111;
  else if (sResonance.find("RSGravitonGG_k0p3_fat30_ak5") != std::string::npos) iResonance = 3311;
  else if (sResonance.find("RSGravitonGG_k0p5_fat30_ak5") != std::string::npos) iResonance = 3511;
  else if (sResonance.find("RSGravitonGG_k0p7_fat30_ak5") != std::string::npos) iResonance = 3711;
  else if (sResonance.find("RSGravitonGG_k0p9_fat30_ak5") != std::string::npos) iResonance = 3911;

  else if (sResonance.find("RSGravitonQQ_k0p1_fat30_ak5") != std::string::npos) iResonance = 3112;
  else if (sResonance.find("RSGravitonQQ_k0p3_fat30_ak5") != std::string::npos) iResonance = 3312;
  else if (sResonance.find("RSGravitonQQ_k0p5_fat30_ak5") != std::string::npos) iResonance = 3512;
  else if (sResonance.find("RSGravitonQQ_k0p7_fat30_ak5") != std::string::npos) iResonance = 3712;
  else if (sResonance.find("RSGravitonQQ_k0p9_fat30_ak5") != std::string::npos) iResonance = 3912;

  else if (sResonance.find("RSGraviton_2012_Z2star_ak5_GGtoGG_fat30") != std::string::npos) iResonance = 4011;
  else if (sResonance.find("RSGraviton_2012_Z2star_ak5_QQtoQQ_fat30") != std::string::npos) iResonance = 4012;
  else if (sResonance.find("Qstar_2012_Z2star_ak5_fat30") != std::string::npos) iResonance = 4013;

  else if (sResonance.find("RSGraviton_2012_D6T_ak5_GGtoGG_fat30") != std::string::npos) iResonance = 4014;
  else if (sResonance.find("RSGraviton_2012_D6T_ak5_QQtoQQ_fat30") != std::string::npos) iResonance = 4015;
  else if (sResonance.find("Qstar_2012_D6T_ak5_fat30") != std::string::npos) iResonance = 4016;
  else if (sResonance.find("RSGraviton_2012_D6T_ak5_fat30") != std::string::npos) iResonance = 4017;

  if (iResonance > 20 && iResonance < 29)  {
    DATASETFN="../data/PFDiFatJetMassList_DATA.txt";
    //    LUMI = 117.6;
    LUMI = 130.0;
    MININVMASS = 526.;
    MAXINVMASS = 3019.;
    JESERROR=0.032;   // relative error on JES
    JERERROR=0.11;   // relative error on JER

    
  }

  if (iResonance > 120 && iResonance < 129)  {
    DATASETFN="../data/PFDiJetMassList_DATA.txt";
    LUMI = 117.6;
    MININVMASS = 526.;
    MAXINVMASS = 3019.;
    JESERROR=0.032;   // relative error on JES
    JERERROR=0.11;   // relative error on JER
  }
     
  if (iResonance > 1000 && iResonance < 1999)  {
    MININVMASS = 890.;
    MAXINVMASS = 5500.;
  }


  if (iResonance > 200 && iResonance < 210)  {
    DATASETFN="../data/ds_data_2011-nominal.txt";
    MININVMASS = 500.;
    MAXINVMASS = 2500.;
//    LUMIERROR = sqrt(pow(LUMIERROR,2) + pow(0.13,2));
  }

  if (iResonance > 2000 && iResonance < 2010)  {
    DATASETFN="../data/lumi46fb_dataMC_data_1JetMassTag.txt";
    MININVMASS = 890.;
    MAXINVMASS = 3279.;
    LUMIERROR = sqrt(pow(LUMIERROR,2) + pow(0.09,2));
  }
  if (iResonance > 2010 && iResonance < 2020)  {
    DATASETFN="../data/lumi46fb_dataMC_data_2JetMassTag.txt";
    MININVMASS = 890.;
    MAXINVMASS = 2037.;
    LUMIERROR = sqrt(pow(LUMIERROR,2) + pow(0.18,2));
  }

  if (iResonance > 4000 && iResonance < 5000)  {
    DATASETFN="../data/dijet_mass_fat_3949pbm1_fatMass_190456_195947.txt";
    MININVMASS = 890.;
    MAXINVMASS = 4869.;
    //MAXINVMASS = 4686.;
    LUMI = 3950.;
    LUMIERROR=0.050; // relative error on luminosity
    JESERROR=0.0125;   // relative error on JES
  }



  // setup label
  char label[100];
  sprintf(label, "%2.0f_%d", signalMass, statlevel);

  // setup workspace
  RooWorkspace *ws = new RooWorkspace("ws");

  // observable
  ws->factory("invmass[0]");
  RooRealVar* invmass=ws->var("invmass");
  invmass->setRange(MININVMASS, MAXINVMASS);

  cout << "cResonance = " << cResonance << " iResonance is = " << iResonance << endl;
  cout << "MININVMASS = " << MININVMASS << " MAXINVMASS = " << MAXINVMASS << " Lumi = " << LUMI << endl;
  cout << "Verbose = " << verbose_ << endl;

  // background
  int number_of_parameters = 4;

  if (statlevel==63){
    // swap fita and fitb
    if (iResonance > 2010 && iResonance < 2020)  {
      number_of_parameters = 3;
      ws->factory("EXPR::background('pow(1-invmass/7000.0+p3*(invmass/7000.0)*(invmass/7000.0),p1)/pow(invmass/7000,p2)',p1[5.1,0,100],p2[5.72,-100,100], p3[-0.0547,-1,1],invmass)");
      ws->factory("EXPR::backgroundb('pow(1.0-invmass/7000.0,pb1)/pow(invmass/7000.0,pb2+pb3*0)', pb1[12,-30,30], pb2[2,-20,20], pb3[0,0,0],invmass)");
      ws->factory("EXPR::backgroundc('pow(1-invmass/7000.0,0)/pow(invmass/7000,pc2)',pc2[5.4,-100,100],pc3[0,0,0],pc1[0,0,0],invmass)");
      //    ws->factory("EXPR::backgroundc('pow(1-invmass/7000.0,pc1)/pow(invmass/7000,0)',pc1[7.8,-100,100],pc2[0,0,0],pc3[0,0,0],invmass)");
    } else {
      number_of_parameters = 4;
      ws->factory("EXPR::background('pow(1-invmass/7000.0+p3*(invmass/7000.0)*(invmass/7000.0),p1)/pow(invmass/7000,p2)',p1[8.41,0,100],p2[5.20,-100,100], p3[-0.017,-1,1],invmass)");
      ws->factory("EXPR::backgroundb('pow(1.0-invmass/7000.0,pb1)/pow(invmass/7000.0,pb2+pb3*log(invmass/7000.0))', pb1[7.460,-30,30], pb2[5.882,-20,20], pb3[0.106,-5,5],invmass)");
      ws->factory("EXPR::backgroundc('pow(1-invmass/7000.0,pc1)/pow(invmass/7000,pc2)',pc1[7.8,-100,100],pc2[5.4,-100,100],pc3[0,0,0],invmass)");
    }
  } else {
    if ((statlevel==64)||(LESS_PARAMETERS)){
      // swap fita and fitc
      if (iResonance > 2010 && iResonance < 2020)  {
	number_of_parameters = 2;
	ws->factory("EXPR::background('pow(1-invmass/7000.0,0)/pow(invmass/7000,p2)',p2[5.4,-100,100],p3[0,0,0],p1[0,0,0],invmass)");
	//    ws->factory("EXPR::background('pow(1-invmass/7000.0,p1)/pow(invmass/7000,0)',p1[7.8,-100,100],p2[0,0,0],p3[0,0,0],invmass)");
	ws->factory("EXPR::backgroundb('pow(1-invmass/7000.0+pb3*(invmass/7000.0)*(invmass/7000.0),pb1)/pow(invmass/7000,pb2)',pb1[5.1,0,100],pb2[5.72,-100,100], pb3[-0.0547,-1,1],invmass)");
	ws->factory("EXPR::backgroundc('pow(1.0-invmass/7000.0,pc1)/pow(invmass/7000.0,pc2+pc3*0)', pc1[12,-30,30], pc2[2,-20,20], pc3[0,0,0],invmass)");
      } else {
	number_of_parameters = 3;
	ws->factory("EXPR::background('pow(1-invmass/7000.0,p1)/pow(invmass/7000,p2)',p1[7.8,-100,100],p2[5.4,-100,100],p3[0,0,0],invmass)");
	ws->factory("EXPR::backgroundb('pow(1-invmass/7000.0+pb3*(invmass/7000.0)*(invmass/7000.0),pb1)/pow(invmass/7000,pb2)',pb1[8.41,0,100],pb2[5.20,-100,100], pb3[-0.017,-1,1],invmass)");
	ws->factory("EXPR::backgroundc('pow(1.0-invmass/7000.0,pc1)/pow(invmass/7000.0,pc2+pc3*log(invmass/7000.0))', pc1[7.460,-30,30], pc2[5.882,-20,20], pc3[0.106,-5,5],invmass)");
      }
    } else {
      if (iResonance > 2010 && iResonance < 2020)  {
	number_of_parameters = 3;
	ws->factory("EXPR::background('pow(1.0-invmass/7000.0,p1)/pow(invmass/7000.0,p2+p3*0)', p1[12,-30,30], p2[2,-20,20], p3[0,0,0],invmass)");
	ws->factory("EXPR::backgroundb('pow(1-invmass/7000.0+pb3*(invmass/7000.0)*(invmass/7000.0),pb1)/pow(invmass/7000,pb2)',pb1[5.1,0,100],pb2[5.72,-100,100], pb3[-0.0547,-1,1],invmass)");
	ws->factory("EXPR::backgroundc('pow(1-invmass/7000.0,0)/pow(invmass/7000,pc2)',pc2[5.4,-100,100],pc3[0,0,0],pc1[0,0,0],invmass)");
	//    ws->factory("EXPR::backgroundc('pow(1-invmass/7000.0,pc1)/pow(invmass/7000,0)',pc1[7.8,-100,100],pc2[0,0,0],pc3[0,0,0],invmass)");
      } else {
	number_of_parameters = 4;
	if (iResonance > 4010 && iResonance < 4020)  {
	  
	  ws->factory("EXPR::background('pow(1.0-invmass/8000.0,p1)/pow(invmass/8000.0,p2+p3*log(invmass/8000.0))', p1[7.460,-30,30], p2[5.882,-20,20], p3[0.106,-5,5],invmass)");
	  ws->factory("EXPR::backgroundb('pow(1-invmass/8000.0+pb3*(invmass/8000.0)*(invmass/8000.0),pb1)/pow(invmass/8000,pb2)',pb1[8.41,0,100],pb2[5.20,-100,100], pb3[-0.017,-1,1],invmass)");
	  ws->factory("EXPR::backgroundc('pow(1-invmass/8000.0,pc1)/pow(invmass/8000,pc2)',pc1[7.8,-100,100],pc2[5.4,-100,100],pc3[0,0,0],invmass)");
	} else {
	  
	  ws->factory("EXPR::background('pow(1.0-invmass/7000.0,p1)/pow(invmass/7000.0,p2+p3*log(invmass/7000.0))', p1[7.460,-30,30], p2[5.882,-20,20], p3[0.106,-5,5],invmass)");
	  ws->factory("EXPR::backgroundb('pow(1-invmass/7000.0+pb3*(invmass/7000.0)*(invmass/7000.0),pb1)/pow(invmass/7000,pb2)',pb1[8.41,0,100],pb2[5.20,-100,100], pb3[-0.017,-1,1],invmass)");
	  ws->factory("EXPR::backgroundc('pow(1-invmass/7000.0,pc1)/pow(invmass/7000,pc2)',pc1[7.8,-100,100],pc2[5.4,-100,100],pc3[0,0,0],invmass)");
	  
	}


      }
    }
  }
  // data
  int ndatabins=(MAXINVMASS-MININVMASS);
  RooDataHist* binnedData=makeBinnedData(DATASETFN, "binnedData", invmass, ndatabins, MININVMASS, MAXINVMASS);
  ws->import(*binnedData);
  
  // signal rate
  if(USELOGNORM) ws->factory("RooLognormal::lumi_prior(lumi[0], lumiM0[0], lumiK0[1])");
  else           ws->factory("RooGaussian::lumi_prior(lumi[0], lumiM0[0], lumiK0[1])");
  ws->var("lumi")->setVal(LUMI);
  ws->var("lumi")->setRange(std::max(0.0, LUMI-NSIGCUTOFF*LUMI*LUMIERROR), LUMI+NSIGCUTOFF*LUMI*LUMIERROR);
  ws->var("lumi")->setConstant();
  ws->var("lumiM0")->setVal(LUMI);
  if(USELOGNORM) ws->var("lumiK0")->setVal(LUMIERROR+1);
  else           ws->var("lumiK0")->setVal(LUMI*LUMIERROR);
  ws->factory("prod::nsig(xs[0], lumi)");

  ws->factory("iResonance[1]");
  ws->var("iResonance")->setVal(iResonance);

  // signal nuisance parameters
  ws->factory("sigMass[1000]");
  ws->var("sigMass")->setVal(signalMass);
  if(USELOGNORM) ws->factory("RooLognormal::sigMassDelta_prior(sigMassDelta[0], sigMassDeltaM0[0], sigMassDeltaK0[1])");
  else           ws->factory("RooGaussian::sigMassDelta_prior(sigMassDelta[0], sigMassDeltaM0[0], sigMassDeltaK0[1])");
  if(USELOGNORM) ws->factory("RooLognormal::sigWidthDelta_prior(sigWidthDelta[0], sigWidthDeltaM0[0], sigWidthDeltaK0[1])");
  else           ws->factory("RooGaussian::sigWidthDelta_prior(sigWidthDelta[0], sigWidthDeltaM0[0], sigWidthDeltaK0[1])");
  ws->var("sigMassDelta")->setVal(JES);
  ws->var("sigWidthDelta")->setVal(JER);
  ws->var("sigMassDelta")->setRange(std::max(0.0, JES-NSIGCUTOFF*JESERROR), JES+NSIGCUTOFF*JESERROR);
  ws->var("sigWidthDelta")->setRange(std::max(0.0, JER-NSIGCUTOFF*JERERROR), JER+NSIGCUTOFF*JERERROR);
  ws->var("sigMassDeltaM0")->setVal(JES);
  ws->var("sigWidthDeltaM0")->setVal(JER);
  if(USELOGNORM) ws->var("sigMassDeltaK0")->setVal(JESERROR+1);
  else           ws->var("sigMassDeltaK0")->setVal(JESERROR);
  if(USELOGNORM) ws->var("sigWidthDeltaK0")->setVal(JERERROR+1);
  else           ws->var("sigWidthDeltaK0")->setVal(JERERROR);
  ws->var("sigMassDelta")->setConstant(true);
  ws->var("sigWidthDelta")->setConstant(true);


  std::cout << "================================"  << ws->var("iResonance")->getVal()  << "================================" << std::endl;
  // signal shape
  ws->factory("Qstar_qg_3::signal(invmass, sigMassDelta, sigWidthDelta, sigMass, iResonance)");

  // model
  //  ws->factory("SUM::modela(nsig*signal, nbkg[474872,270000,670000]*background)"); // background HT-calo
  //  ws->factory("SUM::modela(nsig*signal, nbkg[346573,140000,540000]*background)"); // background HT-PF
  //  ws->factory("SUM::modela(nsig*signal, nbkg[319282,100,5100000]*background)"); // background HT-fat
  //  ws->factory("SUM::modela(nsig*signal, nbkg[250481,100000,400000]*background)"); // background Jet-calo
  //  ws->factory("SUM::modela(nsig*signal, nbkg[181374,0,300000]*background)"); // background Jet-PF
  //  ws->factory("SUM::modela(nsig*signal, nbkg[87242,0,200000]*background)"); // background Jet-fat
  ws->factory("SUM::modela(nsig*signal, nbkg[1000,0,1E8]*background)");
  ws->factory("SUM::modelb(nsig*signal, nbkg*backgroundb)");
  ws->factory("SUM::modelc(nsig*signal, nbkg*backgroundc)");
  ws->defineSet("observables","invmass");
  ws->defineSet("POI","xs");

  if(USEJEFFREYS)
  {
      ws->factory("RooJeffreysPrior::xs_prior(modela,xs,invmass)");
      ws->pdf("xs_prior")->specialIntegratorConfig(kTRUE)->getConfigSection("RooIntegrator1D").setRealValue("maxSteps",3);
  }
  else ws->factory("RooUniform::xs_prior(xs)");

  if(statlevel==0 || statlevel== 2000 || statlevel == 2100) ws->factory("PROD::prior(xs_prior)");
  if(statlevel==1) ws->factory("PROD::prior(xs_prior)");
  if(statlevel==2 || statlevel==1002) ws->factory("PROD::prior(xs_prior,lumi_prior)");
  if(statlevel==3 || statlevel==1003) ws->factory("PROD::prior(xs_prior,sigMassDelta_prior)");
  if(statlevel==4 || statlevel==1004) ws->factory("PROD::prior(xs_prior,sigWidthDelta_prior)");
  if(statlevel==5) ws->factory("PROD::prior(xs_prior,lumi_prior,sigMassDelta_prior)");
  if(statlevel==6 || statlevel==63 || statlevel==64) ws->factory("PROD::prior(xs_prior,lumi_prior,sigMassDelta_prior,sigWidthDelta_prior)");
  if(statlevel==7) ws->factory("PROD::prior(xs_prior,sigMassDelta_prior)");
  if(statlevel==8) ws->factory("PROD::prior(xs_prior,lumi_prior,sigMassDelta_prior)");
  if(statlevel==9) ws->factory("PROD::prior(xs_prior,lumi_prior)");
  if(statlevel==11 || statlevel==12 || statlevel==13 || statlevel==14) ws->factory("PROD::prior(xs_prior)");
  if(statlevel>=100 && statlevel < 1000) ws->factory("PROD::prior(xs_prior)");

  if(statlevel==0 || statlevel== 2000 || statlevel == 2100) ws->defineSet("nuisSet","");
  if(statlevel==1) ws->defineSet("nuisSet","");
  if(statlevel==2 || statlevel==1002) ws->defineSet("nuisSet","lumi");
  if(statlevel==3 || statlevel==1003) ws->defineSet("nuisSet","sigMassDelta");
  if(statlevel==4 || statlevel==1004) ws->defineSet("nuisSet","sigWidthDelta");
  if(statlevel==5) ws->defineSet("nuisSet","lumi,sigMassDelta");
  if(statlevel==6 || statlevel==63 || statlevel==64) ws->defineSet("nuisSet","lumi,sigMassDelta,sigWidthDelta");
  if(statlevel==7) ws->defineSet("nuisSet","sigMassDelta");
  if(statlevel==8) ws->defineSet("nuisSet","lumi,sigMassDelta");
  if(statlevel==9) ws->defineSet("nuisSet","lumi");
  if(statlevel==11 || statlevel==12 || statlevel==13 || statlevel==14) ws->defineSet("nuisSet","");
  if(statlevel>=100 && statlevel < 1000) ws->defineSet("nuisSet","");

  // do a background-only fit first
  // exclude window +20% -20% units in width
  double mininvmass=ws->var("invmass")->getMin();
  double maxinvmass=ws->var("invmass")->getMax();
  cout << "mininvmass = " << mininvmass << " maxinvmass = " << maxinvmass << endl;
  ws->var("invmass")->setRange("FULL", mininvmass,maxinvmass);
  RooFitResult* fit_for_covariance;
  RooFitResult* fit;

  cout << "Do fit A" << endl;
  fit_for_covariance=doFit(std::string("bfita")+label, ws->pdf("modela"), binnedData, invmass, ws->function("nsig"), ws->var("nbkg"), NBINS-1, BOUNDARIES, "FULL", 0, verbose_);

  cout << "Do fit B" << endl;
  fit=doFit(std::string("bfitb")+label, ws->pdf("modelb"), binnedData, invmass, ws->function("nsig"), ws->var("nbkg"), NBINS-1, BOUNDARIES, "FULL", 0, verbose_);

  cout << "Do fit C" << endl;
  fit=doFit(std::string("bfitc")+label, ws->pdf("modelc"), binnedData, invmass, ws->function("nsig"), ws->var("nbkg"), NBINS-1, BOUNDARIES, "FULL", 0, verbose_);

  // integrate over model to get xs estimate as input to the B+S fit
  double pdfIntegral=ws->var("nbkg")->getVal()*calcPDF1DIntegral(ws->pdf("modela"), invmass, signalMass*0.9, signalMass*1.1);
  double maxXS = sqrt(pdfIntegral)*5/ws->var("lumi")->getVal();
  RooRealVar* xs=ws->var("xs");
  maxXS = maxXS*10.0;
  if(ALLOWSNEGATIVE)
      xs->setRange(-maxXS*1000.0,maxXS*1000.0);
  else    
      xs->setRange(0,maxXS*1000.0);
  xs->setVal(maxXS/1.0);

  cout << "doing S+B fit to estimate background" << endl;

  if(!doAggressiveBkgFit) {
        // Fit s+b with signal floating
        xs->setConstant(false);
	fit=doFit(std::string("bsfita")+label, ws->pdf("modela"), binnedData, invmass, ws->function("nsig"), ws->var("nbkg"), NBINS-1, BOUNDARIES, "FULL", 0, verbose_);
        // Rerun s+b fit with fixed signal, to get a proper covariance matrix for the bg parameters
        cout << "rerun S+B fit with fixed S for covariance matrix" << endl;
        xs->setConstant(true);
        fit_for_covariance=doFit(std::string("bfita")+label, ws->pdf("modela"), binnedData, invmass, ws->function("nsig"), ws->var("nbkg"), NBINS-1, BOUNDARIES, "FULL", 0, verbose_);
        xs->setConstant(false);
  }

  cout << "start limit setting" << endl;

  double nbkgValInit=ws->var("nbkg")->getVal();
  double p1ValInit=ws->var("p1")->getVal();
  double p2ValInit=ws->var("p2")->getVal();
  double p3ValInit=ws->var("p3")->getVal();


  for(int PE=0; PE<=numPEs; PE++) {

    cout << "Start to loop over PEs" << endl;

    if(numPEs>0 && PE==0) continue; // skip the 0th entry if we are running PEs

    system( "ps -v | grep RSS" );
    system( "ps -v | grep dijetStats" );
    
    //    RooDataHist* binnedData;
    // replace with pseudodata if we are doing more than zero PE's
    if(numPEs>0) {
      ws->var("nbkg")->setVal(nbkgValInit);
      ws->var("p1")->setVal(p1ValInit);
      ws->var("p2")->setVal(p2ValInit);
      ws->var("p3")->setVal(p3ValInit);
      binnedData=makeBinnedPseudoData(ws->pdf("background"), ws->var("nbkg")->getVal(), "binnedData", invmass, ndatabins, MININVMASS, MAXINVMASS);
      ws->import(*binnedData, kTRUE);
    }
    
    // do the background+signal fit
    xs->setConstant(false);
    char pelabel[120];
    sprintf(pelabel, "%s_pe%d", label, PE);

    if(doAggressiveBkgFit) {
      ws->var("p1")->setConstant(true);
      ws->var("p2")->setConstant(true);
      ws->var("p3")->setConstant(true);
      ws->var("pb1")->setConstant(true);
      ws->var("pb2")->setConstant(true);
      ws->var("pb3")->setConstant(true);
      ws->var("pc1")->setConstant(true);
      ws->var("pc2")->setConstant(true);
      ws->var("pc3")->setConstant(true);
      ws->var("nbkg")->setConstant(true);
      
      fit=doFit(std::string("bsfita")+pelabel, ws->pdf("modela"), binnedData, invmass, ws->function("nsig"), ws->var("nbkg"), NBINS-1, BOUNDARIES, "FULL", 0, verbose_);
    } else
    if (statlevel == 2100) {
        // Fit b with excluded signal region
	double minexclude = std::max(mininvmass, ws->var("sigMass")->getVal()*0.8);
	double maxexclude = std::min(maxinvmass, ws->var("sigMass")->getVal()*1.1);
	ws->var("invmass")->setRange("SB1", mininvmass, minexclude);
	if (maxexclude <  maxinvmass){
	  ws->var("invmass")->setRange("SB2", maxexclude, maxinvmass);
	  fit=doFit(std::string("bfita")+label, ws->pdf("modela"), binnedData, invmass, ws->function("nsig"), ws->var("nbkg"), NBINS-1, BOUNDARIES, "SB1,SB2");
	  ws->var("invmass")->removeRange("SB2");
	} else {
	  fit=doFit(std::string("bfita")+label, ws->pdf("modela"), binnedData, invmass, ws->function("nsig"), ws->var("nbkg"), NBINS-1, BOUNDARIES, "SB1");
	}

	ws->var("invmass")->removeRange("SB1");

	ws->var("p1")->setConstant(true);
	ws->var("p2")->setConstant(true);
	ws->var("p3")->setConstant(true);
	ws->var("pb1")->setConstant(true);
	ws->var("pb2")->setConstant(true);
	ws->var("pb3")->setConstant(true);
	ws->var("pc1")->setConstant(true);
	ws->var("pc2")->setConstant(true);
	ws->var("pc3")->setConstant(true);
	ws->var("nbkg")->setConstant(true);
    } else
      if (PE!=0){
	if(ALLOWSNEGATIVE)
          xs->setRange(-maxXS*1000.0,maxXS*1000.0);
	else    
          xs->setRange(0,maxXS*1000.0);
	xs->setVal(maxXS/1.0);
	
	fit=doFit(std::string("bsfita")+pelabel, ws->pdf("modela"), binnedData, invmass, ws->function("nsig"), ws->var("nbkg"), NBINS-1, BOUNDARIES, "FULL", 0, verbose_);

      }

    // set parameters for limit calculation
    if(statlevel==1 || statlevel==11 || statlevel==12 || statlevel==13 || statlevel==14 || (statlevel>=100 && statlevel < 1000)) {
    } else if(statlevel==2 || statlevel==1002) {
      ws->var("lumi")->setConstant(false);
    } else if(statlevel==3 || statlevel==1003) {
      ws->var("sigMassDelta")->setConstant(false);
    } else if(statlevel==4 || statlevel==1004) {
      ws->var("sigWidthDelta")->setConstant(false);
    } else if(statlevel==5) {
      ws->var("lumi")->setConstant(false);
      ws->var("sigMassDelta")->setConstant(false);
    } else if(statlevel==6 || statlevel==63 || statlevel==64) {
      ws->var("lumi")->setConstant(false);
      ws->var("sigMassDelta")->setConstant(false);
      ws->var("sigWidthDelta")->setConstant(false);
    } else if(statlevel==7) {
      ws->var("sigMassDelta")->setConstant(false);
    } else if(statlevel==8) {
      ws->var("lumi")->setConstant(false);
      ws->var("sigMassDelta")->setConstant(false);
    } else if(statlevel==9) {
      ws->var("lumi")->setConstant(false);
    }
    
    cout << "Set here the value of the signal for final calculation" << endl;

    if(xs->getVal()<0)
        xs->setVal(0);
    xs->setRange(0,maxXS);

    // setup model config
    ModelConfig modelConfigA("modelConfigA");
    modelConfigA.SetWorkspace(*ws);
    modelConfigA.SetPdf(*ws->pdf("modela"));
    modelConfigA.SetPriorPdf(*ws->pdf("prior"));
    modelConfigA.SetParametersOfInterest(*ws->set("POI"));
    modelConfigA.SetNuisanceParameters(*ws->set("nuisSet"));
    ModelConfig modelConfigB("modelConfigB");
    modelConfigB.SetWorkspace(*ws);
    modelConfigB.SetPdf(*ws->pdf("modelb"));
    modelConfigB.SetPriorPdf(*ws->pdf("prior"));
    modelConfigB.SetParametersOfInterest(*ws->set("POI"));
    modelConfigB.SetNuisanceParameters(*ws->set("nuisSet"));
    ModelConfig modelConfigC("modelConfigC");
    modelConfigC.SetWorkspace(*ws);
    modelConfigC.SetPdf(*ws->pdf("modelc"));
    modelConfigC.SetPriorPdf(*ws->pdf("prior"));
    modelConfigC.SetParametersOfInterest(*ws->set("POI"));
    modelConfigC.SetNuisanceParameters(*ws->set("nuisSet"));
    
    ws->import(modelConfigA);
    ws->import(modelConfigB);
    ws->import(modelConfigC);
    ws->Print();
    printVal(*ws->var("xs"));
    printVal(*ws->var("nbkg"));
    printVal(*ws->var("lumi"));
    printVal(*ws->var("lumiM0"));
    printVal(*ws->var("lumiK0"));
    printVal(*ws->var("sigMassDelta"));
    printVal(*ws->var("sigMassDeltaM0"));
    printVal(*ws->var("sigMassDeltaK0"));
    printVal(*ws->var("sigWidthDelta"));
    printVal(*ws->var("sigWidthDeltaM0"));
    printVal(*ws->var("sigWidthDeltaK0"));
    printVal(*ws->var("p1"));
    printVal(*ws->var("p2"));
    printVal(*ws->var("p3"));
    printVal(*ws->var("pb1"));
    printVal(*ws->var("pb2"));
    printVal(*ws->var("pb3"));
    printVal(*ws->var("pc1"));
    printVal(*ws->var("pc2"));
    printVal(*ws->var("pc3"));
    if (verbose_) ws->writeToFile("ws.root");
    else if (!verbose_ && WRITE){
      string title("ws");
      title = title + "_" + sResonance + "_" + Form("%f", signalMass) + "_" + Form("%d",statlevel) + ".root";
      ws->writeToFile(title.c_str());
    }

    double lower=-1, upper=-1;
    //    int niters=1000;
    //    int niters=200;
    int niters=30;
    double alpha=0.05;
    double lstail=0.0;
    JPMCCalculator mcA(*binnedData, modelConfigA);
    mcA.SetNumIterations(niters);
    mcA.SetConfidenceLevel(1-alpha);
    mcA.SetLeftSideTailFraction(lstail);
    JPMCCalculator mcB(*binnedData, modelConfigB);
    mcB.SetNumIterations(niters);
    mcB.SetConfidenceLevel(1-alpha);
    mcB.SetLeftSideTailFraction(lstail);
    JPMCCalculator mcC(*binnedData, modelConfigC);
    mcC.SetNumIterations(niters);
    mcC.SetConfidenceLevel(1-alpha);
    mcC.SetLeftSideTailFraction(lstail);
    
    double p1val=ws->var("p1")->getVal(); //double p1err=ws->var("p1")->getError();
    double p2val=ws->var("p2")->getVal(); //double p2err=ws->var("p2")->getError();
    double p3val=ws->var("p3")->getVal(); //double p3err=ws->var("p3")->getError();


    TH1D* histA=dynamic_cast<TH1D*>(mcA.GetPosteriorHist()->Clone("histA"));

    if(statlevel==1 || statlevel==5 || statlevel==6 || statlevel==7 || statlevel==9 || statlevel==11 || statlevel==12 || statlevel==13 || statlevel==14 || statlevel==63 || statlevel==64 || (statlevel>=100 &&  statlevel<1000)) {

      if (statlevel==11){
        TH1D* histB=dynamic_cast<TH1D*>(mcB.GetPosteriorHist()->Clone("histB"));
        TH1D* histC=dynamic_cast<TH1D*>(mcC.GetPosteriorHist()->Clone("histC"));
	histA->Add(histB);
	histA->Add(histC);
	delete histB;
	delete histC;
      }
      
      else if (statlevel==12){
	TH1D* histB=dynamic_cast<TH1D*>(mcB.GetPosteriorHist()->Clone("histB"));
	histA->Add(histB);
	delete histB;
      }
      
      else if (statlevel==13){
	TH1D* histB=dynamic_cast<TH1D*>(mcB.GetPosteriorHist()->Clone("histB"));
	delete histA;
	histA=histB;
      }
      
      else if (statlevel==14){
	TH1D* histC=dynamic_cast<TH1D*>(mcC.GetPosteriorHist()->Clone("histC"));
	delete histA;
	histA=histC;
      }
      
      else {
        // Code taken from TFitResult.RandomizePars
        Int_t nPar = number_of_parameters; // fit_for_covariance->floatParsFinal().getSize();
        // calculate the elements of the upper-triangular matrix L that gives Lt*L = C
        // where Lt is the transpose of L (the "square-root method")
        TMatrix L(nPar,nPar);
        for(Int_t iPar= 0; iPar < nPar; iPar++) {
     	  // calculate the diagonal term first
     	  L(iPar,iPar)= fit_for_covariance->covarianceMatrix()(iPar,iPar);
     	  for(Int_t k= 0; k < iPar; k++) {
     	    Double_t tmp= L(k,iPar);
     	    L(iPar,iPar)-= tmp*tmp;
     	  }
     	  L(iPar,iPar)= sqrt(L(iPar,iPar));
     	  // then the off-diagonal terms
     	  for(Int_t jPar= iPar+1; jPar < nPar; jPar++) {
     	    L(iPar,jPar)= fit_for_covariance->covarianceMatrix()(iPar,jPar);
     	    for(Int_t k= 0; k < iPar; k++) {
     	      L(iPar,jPar)-= L(k,iPar)*L(k,jPar);
     	    }
     	    L(iPar,jPar)/= L(iPar,iPar);
     	  }
        }
        // remember Lt
        TMatrix* _Lt= new TMatrix(TMatrix::kTransposed,L);
        TVectorD eigenValues(4);
        TMatrixD eigenVectors=fit_for_covariance->covarianceMatrix().EigenVectors(eigenValues);
        std::cout << "EigenVectors" << std::endl;
        for(Int_t k= 0; k < nPar; k++) {
     	  std::cout << k << ": ";
     	  for(Int_t l= 0; l < nPar; l++) {
     	    std::cout << eigenVectors[l][k] << " ";
     	  }
     	  std::cout << std::endl;
        }
        eigenValues.Sqrt(); // Turn variances into errors 
 
        if (statlevel<100) {
         histA->Scale(3); // 3 times the central value plus up/down variations along 3 axes
         TVector g(nPar);
	 for(Int_t v=1; v<nPar;v++){
          if(USENEWCOVARIANCE) {
	    for(Int_t k= 0; k < nPar; k++) {
	      if (eigenVectors[0][v] != 0) 
		g(k)=eigenValues(v)*eigenVectors[k][v]*(eigenVectors[0][v])/fabs(eigenVectors[0][v]);
	      else g(k)=eigenValues(v)*eigenVectors[k][v];
	    }
	  } else {
	      for(Int_t k= 0; k < nPar; k++) g(k)=eigenVectors[v][k];
              // multiply this vector by Lt to introduce the appropriate correlations
              g*= (*_Lt);
	  }
	  cout << "Up iPar = " << nPar << endl;
	  cout << "p1val+g(1) = " <<  p1val+g(1) << endl;
	  cout << "p2val+g(2) = " <<  p2val+g(2) << endl;
	  cout << "p3val+g(3) = " <<  p3val+g(3) << endl;

	  if(g(1)<0)
	      for(Int_t k= 0; k < nPar; k++) g(k)=-g(k);

	  ws->var("nbkg")->setVal(nbkgValInit);
	  ws->var("p1")->setVal(p1ValInit+g(1));
	  ws->var("p2")->setVal(p2ValInit+g(2));
	  ws->var("p3")->setVal(p3ValInit+g(3));

	  TH1D* histAHi=dynamic_cast<TH1D*>(mcA.GetPosteriorHistForce()->Clone("histAHi"));
	  histA->Add(histAHi);
	  delete histAHi;
          if(USENEWCOVARIANCE)
          {
	    for(Int_t k= 0; k < nPar; k++) {
	      if (eigenVectors[0][v] != 0) g(k)= -eigenValues(v)*eigenVectors[k][v]*(eigenVectors[0][v])/fabs(eigenVectors[0][v]); 
	      else g(k)= -eigenValues(v)*eigenVectors[k][v];
	    }
	  } else {
	    for(Int_t k= 0; k < nPar; k++) g(k)=-eigenVectors[v][k];
	    // multiply this vector by Lt to introduce the appropriate correlations
	    g*= (*_Lt);
	  }

	  cout << "Do iPar = " << nPar << endl;
	  cout << "p1val+g(1) = " <<  p1val+g(1) << endl;
	  cout << "p2val+g(2) = " <<  p2val+g(2) << endl;
	  cout << "p3val+g(3) = " <<  p3val+g(3) << endl;

	  if(g(1)>=0)
	    for(Int_t k= 0; k < nPar; k++) g(k)=-g(k);
	  ws->var("nbkg")->setVal(nbkgValInit);
	  ws->var("p1")->setVal(p1ValInit+g(1));
	  ws->var("p2")->setVal(p2ValInit+g(2));
	  ws->var("p3")->setVal(p3ValInit+g(3));

	  TH1D* histALo=dynamic_cast<TH1D*>(mcA.GetPosteriorHistForce()->Clone("histALo"));
	  histA->Add(histALo);
	  delete histALo;
	 }
	}
	
	else if (statlevel>=100 && statlevel < 1000) {
          TVector g(nPar);
	  if ((statlevel>=100) && (statlevel<=102)) {
            if(USENEWCOVARIANCE)
            {
	        for(Int_t k= 0; k < nPar; k++) g(k)=eigenValues(statlevel-99)*eigenVectors[k][statlevel-99]; 
	    } else {
                for(Int_t k= 0; k < nPar; k++) g(k)=eigenVectors[statlevel-99][k];
                // multiply this vector by Lt to introduce the appropriate correlations
                g*= (*_Lt);
	    }
            if(g(1)<0)
	        for(Int_t k= 0; k < nPar; k++) g(k)=-g(k);
	  } else if ((statlevel>=103) && (statlevel<=105)) {
            if(USENEWCOVARIANCE)
            {
	        for(Int_t k= 0; k < nPar; k++) g(k)=-eigenValues(statlevel-102)*eigenVectors[k][statlevel-102]; 
	    } else {
                for(Int_t k= 0; k < nPar; k++) g(k)=-eigenVectors[statlevel-102][k];
                // multiply this vector by Lt to introduce the appropriate correlations
                g*= (*_Lt);
	    }
            if(g(1)>=0)
	        for(Int_t k= 0; k < nPar; k++) g(k)=-g(k);
	  }
	  ws->var("nbkg")->setVal(nbkgValInit);
	  ws->var("p1")->setVal(p1ValInit+g(1));
	  ws->var("p2")->setVal(p2ValInit+g(2));
	  ws->var("p3")->setVal(p3ValInit+g(3));
	  TH1D* histAHi=dynamic_cast<TH1D*>(mcA.GetPosteriorHistForce()->Clone("histAHi"));
	  delete histA;
	  histA=histAHi;
	}
      
      }
      /* Old default with only leading eigenvector
      {
	ws->var("p1")->setVal(std::max(0.0,p1val-p1err));
	ws->var("p2")->setVal(p2val+p2err);
	ws->var("p3")->setVal(p3val+p3err);
	TH1D* histAHi=dynamic_cast<TH1D*>(mcA.GetPosteriorHistForce()->Clone("histAHi"));
	ws->var("p1")->setVal(std::max(0.0,p1val+p1err));
	ws->var("p2")->setVal(p2val-p2err);
	ws->var("p3")->setVal(p3val-p3err);
	TH1D* histALo=dynamic_cast<TH1D*>(mcA.GetPosteriorHistForce()->Clone("histALo"));

	histA->Add(histAHi);
	histA->Add(histALo);

	delete histAHi;
	delete histALo;
      }
      */
 
    }
    
 
    TH1* hPosteriorHist=histA;

    if (verbose_) {
      TCanvas* canvas = new TCanvas("mcPost","Posterior Distribution",500,500);
      hPosteriorHist->Draw();
      canvas->SaveAs(TString("MCpost")+pelabel+".gif");
      delete canvas;
    }

    // Calculate the interval by hand
    double lowerCutOff = lstail*alpha;
    double upperCutOff = 1.-(1.-lstail)*alpha;
    TH1D* hCdf = (TH1D*)hPosteriorHist->Clone("hCdf");
    for(int bin=1; bin<=hCdf->GetNbinsX()+1; ++bin) {
      double last=hCdf->GetBinContent(bin-1);
      double next=hPosteriorHist->GetBinContent(bin);
      hCdf->SetBinContent(bin, last+next);
    }
    hCdf->Scale(1./hPosteriorHist->GetSumOfWeights());
    
    for(int bin=1; bin<=hCdf->GetNbinsX(); bin++) {
      if(lowerCutOff<=hCdf->GetBinContent(bin)) {
	lower=hCdf->GetBinLowEdge(bin);
	break;
      }
    }
    
    for(int bin=hCdf->GetNbinsX(); bin>=1; --bin) {
      if(upperCutOff>=hCdf->GetBinContent(bin)) {
	upper=hCdf->GetBinLowEdge(bin+1);
	break;
      }
    }
    delete hCdf;
    delete histA;

    // print the results
    cout << "PE = " << PE << " ; "
	 << "m0 = " << signalMass << " ; "
	 << "statlevel = " << statlevel << " ; "
	 << "lowerlimit = " << lower << " ; "
	 << "upperlimit = " << upper << endl;

    if (PE > 0) upper_limits.push_back(upper);
    

    t.Print();

    //    delete binnedData;
    //    mcA.Clear();
   

  }


  double median_ = 0;
  std::pair<double, double> onesigma_;
  std::pair<double, double> twosigma_;

  DijetHelper::getQuantiles(upper_limits, median_, onesigma_, twosigma_);

  cout << "m0 = " << signalMass << " ; "
       << " statlevel = " << statlevel << " ; "
       << " twosigma_lower = " << twosigma_.first << " ; "
       << " onesigma_lower = " << onesigma_.first << " ; "
       << " median = " << median_ << " ; "
       << " onesigma_upper = " << onesigma_.second << " ; "
       << " twosigma_upper = " << twosigma_.second << endl << endl << endl;

  cout << "PEsLimits = [" << endl;
  for (unsigned int i = 0; i < upper_limits.size(); i++){
    cout << upper_limits[i];
    if (i < upper_limits.size()-1) cout << ", "; 
  }

  cout << "];" << endl;

}




