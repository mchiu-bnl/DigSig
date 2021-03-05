#ifndef __DIGANA_H__
#define __DIGANA_H__

#include <TH1.h>
#include <TGraphErrors.h>
#include <stdint.h>
#include "DigSig.h"

class TFile;
class TTree;

class DigAna
{
public:
  DigAna(const int numch, const int nsamp);
  virtual ~DigAna();

  Stat_t OpenRootFile(const char *fname);
  int ProcessEvent(const int entry);

  Stat_t GetEntries() { return nentries; }
  DigSig* GetSig(const int ich) { return &digsig[ich]; }

  /** Invert all the pulses */
  void Invert() { invert = -1; }

  /** Set Channels to Skip */
  void SkipAll();
  void EnableCh(const int ich);
  void SkipCh(const int ich);

  /** Read pedestals from file */
  int  SetPed0FromFile(const char * pedfname);
  /** Fill hists to determine ped0, using [sampmin,sampmax] (inclusive) */
  void FillPed0(const Int_t sampmin = 0, const Int_t sampmax = 2);
  void FillPed0(const Double_t xmin, const Double_t xmax);
  /** Set to use event-by-event ped */
  void SetEventPed0Range(const Int_t minsamp, const Int_t maxsamp);
  void SetEventPed0Range(const Double_t minx, const Double_t maxx);

  void CalcIntegralAroundPeak(const Double_t leftlimit, const Double_t rightlimit);

  void SetTemplateSize(const Int_t nptsx, const Int_t nptsy, const Double_t begt, const Double_t endt);
  void SetTemplateMinMaxGoodADC(const Double_t min, const Double_t max);
  void SetTemplateMinMaxFitRange(const Double_t min, const Double_t max);
  void SetTimeOffset(const Double_t o);
  void FillSplineTemplate();
  void FillFcnTemplate();
  void MakeAndWriteTemplate(const char *savename);
  void ReadTemplate(const char *basename);

  int32_t get_evt() { return f_evt; }
  int32_t get_spillevt() { return f_spillevt; }
  int32_t get_dtstamp() { return f_dtstamp; }

private:
  int nch;
  int nsamples;
  int invert;

  std::vector<int> ch_skip; // channels to skip
  std::vector<DigSig> digsig;

  Stat_t nentries;
  TFile *tfile;
  TTree *ttree;
  Int_t f_evt;
  Int_t f_spillevt;
  Int_t f_dtstamp;
  Float_t f_x[128][1024];  // time or sample
  Float_t f_y[128][1024];  // voltage or adc

  TH1 *hdTime;

  ClassDef(DigAna,0)
};

#endif  // __DIGANA_H__
