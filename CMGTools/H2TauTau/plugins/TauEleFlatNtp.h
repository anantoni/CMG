#ifndef CMGH2TAUTAU_TauEleFlatNtp_h
#define CMGH2TAUTAU_TauEleFlatNtp_h

#include "CMGTools/H2TauTau/plugins/BaseFlatNtp.h"
#include "AnalysisDataFormats/CMGTools/interface/CompoundTypes.h"
#include "AnalysisDataFormats/CMGTools/interface/BaseMET.h"
#include "AnalysisDataFormats/CMGTools/interface/Tau.h"
#include "AnalysisDataFormats/CMGTools/interface/Muon.h"
#include "AnalysisDataFormats/CMGTools/interface/Electron.h"
#include "AnalysisDataFormats/CMGTools/interface/METSignificance.h"

#include "CMGTools/H2TauTau/interface/TriggerEfficiency.h"
#include "CMGTools/H2TauTau/interface/SelectionEfficiency.h"
//#include "CMGTools/H2TauTau/interface/TauRate.h"

#include "CMGTools/Common/interface/RecoilCorrector.h"

#include <TRandom2.h>


class TauEleFlatNtp : public BaseFlatNtp {


public:
  explicit TauEleFlatNtp(const edm::ParameterSet&);
  ~TauEleFlatNtp();

  virtual void beginJob() ;
  virtual void analyze(const edm::Event&  iEvent, const edm::EventSetup& iSetup);
  virtual void endJob() ;

  virtual bool fillVariables(const edm::Event & iEvent, const edm::EventSetup & iSetup);
  virtual bool applySelections();
  virtual bool fill();

protected:

  edm::InputTag diTauTag_;
  edm::InputTag genParticlesTag_;
  edm::InputTag pfJetListTag_;
  edm::InputTag diMuonVetoListTag_;
  edm::Handle< std::vector<cmg::TauEle> > diTauList_;
  std::vector<cmg::TauEle> diTauSelList_;
  const cmg::TauEle * diTauSel_;
  std::vector<const cmg::PFJet * > pfJetList_;
  std::vector<const cmg::PFJet * > pfJetListLC_;
  std::vector<const cmg::PFJet * > pfJetListLepLC_;

  std::vector<const cmg::PFJet * > pfJetListBTag_;
  std::vector<const cmg::PFJet * > pfJetListBTagLC_;

  TriggerEfficiency triggerEff_;
  float triggerEffWeight_;
  SelectionEfficiency selectionEff_;
  float selectionEffWeight_;

  float embeddedGenWeight_;//for tau embedded samples

  int   nditau_;//number of candidates before best candidate selection
  float ditaumass_;
  int   ditaucharge_;
  float ditaueta_;
  float ditaupt_;
  float svfitmass_;

  float taumass_;
  float taupt_;
  float taueta_;
  float tauphi_;
  int   tautruth_;
  float tauehop_;
  float taueop_;
  int   taudecaymode_;
  float taudz_;
  float taudxy_;
  float taux_;
  float tauy_;
  float tauz_;
  int   tauantie_;
  int   tauantiemva_;
  int   tauantimu_;
  int   tauisodisc_;
  int   tauisodiscmva_;
  float tauiso_;
  float tauisomva_;
  float taujetpt_;
  float taujeteta_;

  float mupt_;
  float mueta_;
  float muphi_;
  float muiso_;
  float muisomva_;
  float mudz_;
  float mudxy_;
  float mux_;
  float muy_;
  float muz_;
  int   mujetmatch_;//0= no match, 1 lead jet, 2=subleading jet //check which jet list is being used !
  float mujetpt_;
  float mujeteta_;

  float pftransversemass_;
  float pfmetpt_;
  float pfmetphi_;
  float transversemass_;
  double metpt_;//double needed by recoil corrector
  double metphi_;
  
  int njet_;
  float leadJetPt_;
  float leadJetEta_;
  float leadJetRawFactor_;
  float subleadJetPt_;
  float subleadJetEta_;
  float subleadJetRawFactor_;
  float diJetMass_;
  float diJetDeltaEta_;
  float diJetEta1Eta2_;
  int   njetingap_;
  int nbjet_;
  float leadBJetBTagProb_;
  float leadBJetPt_;
  float leadBJetEta_;



  float muLCleadJetPt_;//jets where only the muon has been removed
  float muLCleadJetEta_;

  int categoryCh_;//
  int categoryMT_;//
  int categoryIso_;//
  int categorySM_;//SM search 

private:
  

  int sampleGenEventType_;
  int sampleTruthEventType_;
  int genEventType_;//1=ZtoEE, 3=ZToMuMu, 5=ZToTauTau, 6=ZToOther, 11=WToENu, 13=WToMuNu, 15=WToENu
  int truthEventType_;//1=ZtoEE, 3=ZToMuMu, 5=ZToTauTau, 6=ZOther 11=WToENu, 13=WToMuNu, 15=WToENu, 16=WOther
  float deltaRTruth_;

  const reco::GenParticle * genBoson_;
  const reco::GenParticle * genBosonL1_;
  const reco::GenParticle * genBosonL2_;

  edm::Handle< std::vector<cmg::Electron> > diLeptonVetoList_;

  void fillPFJetListLC(const cmg::TauEle * cand, std::vector<const cmg::PFJet * > * list, std::vector<const cmg::PFJet * > * listLC);
  void fillPFJetListLepLC(const cmg::TauEle * cand, std::vector<const cmg::PFJet * > * list, std::vector<const cmg::PFJet * > * listLC);
  bool vetoDiLepton();
  int truthMatchTau();

  TRandom2 randEngine_; 
  double randsigma_;
  
  
  RecoilCorrector corrector_;
  int recoilCorreciton_;
  std::string fileCorrectTo_;
  std::string fileZmmData_;
  std::string fileZmmMC_;


  int runSVFit_;

  int counterall_;
  int counterev_;
  int countergen_;
  int counterveto_;
  int counterpresel_;
  int countertaueop_;
  int countertauvtx_;
  int countertaumuveto_;
  int countertaueveto_;
  int countertauiso_;
  int countertaumatch_;
  int countermuvtx_;
  int countermuid_;
  int countermuiso_;
  int countermumatch_;
  int counterditau_;
  int counterbestcand_;
  int countertruth_;
  int counter_;

};


#endif
