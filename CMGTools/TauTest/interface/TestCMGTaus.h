#ifndef TESTCMGTAUS_H_
#define TESTCMGTAUS_H_

#include <iomanip>

#include <TString.h>

class TestCMGTaus {
public:
  
  TestCMGTaus():
    dirname_(""),
    firstfile_(1),
    lastfile_(1){
  };
  TestCMGTaus(TString filename, Int_t firstfile, Int_t lastfile):
    dirname_(filename),
    firstfile_(firstfile),
    lastfile_(lastfile){
  };
  ~TestCMGTaus(){};

  void testTau(TString inputtag="cmgTauSel");


private:
  
  TString dirname_;
  Int_t firstfile_;
  Int_t lastfile_;
  
};



#endif /*TESTCMGTAUS_H_*/
