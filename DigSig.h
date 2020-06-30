#ifndef __DIGSIG_H__
#define __DIGSIG_H__

#include <TH1.h>
//#include <TH2.h>
#include <fstream>
#include <vector>

class TFile;
class TTree;
class TGraphErrors;
class TH2;
//class THnSparse;
class RunningStats;

/**
  
DigSig: Single Channel digital signal class, includes processing

*/

class DigSig
{
public:
  DigSig(const int chnum = 0, const int nsamp = 0);
  virtual ~DigSig();

  void SetY(const Float_t *y, const int invert = 1);
  void SetXY(const Float_t *x, const Float_t *y, const int invert = 1);

  TH1 *GetHist() { return hpulse; }
  TGraphErrors *GetGraph() { return gpulse; }
  Double_t GetAmpl() { return f_ampl; }
  Double_t GetTime() { return f_time; }
  Double_t GetIntegral() { return f_integral; }

  /**
   * Fill hists from data between minsamp and maxsamp bins
   * sample number starts from 0
   * Ped0 is the running pedestal and rms
   * */
  void FillPed0(const Int_t minsamp, const Int_t maxsamp);
  void FillPed0(const Double_t minsamp, const Double_t maxsamp);

  Double_t GetPed0() { return ped0; }
  Double_t GetPed0RMS() { return ped0rms; }

  /** Set the global pedestal. Once set, it is applied to the data for all events.  */
  void SetPed0(const Double_t mean, const Double_t rms = 0.);

  /** Use the event by event pedestal, */
  void SetEventPed0Range(const Int_t minsamp, const Int_t maxsamp) {
    minped0samp = minsamp;
    maxped0samp = maxsamp;
  }
  void SetEventPed0Range(const Double_t minx, const Double_t maxx) {
    minped0x = minx;
    maxped0x = maxx;
  }
  void CalcEventPed0(const Int_t minsamp, const Int_t maxsamp);
  void CalcEventPed0(const Double_t minx, const Double_t maxx);

  /** Leading Edge Discriminator signal */
  Double_t LeadingEdge(const Double_t threshold);  // Leading Edge Discriminator Time

  /** digital CFD, threshold is in fraction of amplitude */
  Double_t dCFD(const Double_t fraction_threshold);
 
  /** Simple integral to get total charge, etc */
  Double_t Integral(const Double_t xmin, const Double_t xmax);

  /** The maximum value from all samples */
  void LocMax(Double_t& x_at_max, Double_t& ymax, Double_t minxrange = 0., Double_t maxxrange = 0.);

  /** The minimum value from all samples (including negatives) */
  void LocMin(Double_t& x_at_min, Double_t& ymin, Double_t minxrange = 0., Double_t maxxrange = 0.);

  /** Use template fit to get ampl and time */
  Int_t    FitTemplate();
  //Double_t Ampl() { return f_ampl; }
  //Double_t Time() { return f_time; }

  /** Make template waveforms for later fits */
  void  SetTemplateSize(const Int_t nptsx, const Int_t nptsy, const Double_t begt, const Double_t endt);
  int   FillSplineTemplate();
  void  FillFcnTemplate();
  void  MakeAndWriteTemplate(std::ostream& out, std::ostream& oerr);
  Int_t ReadTemplate(std::ifstream& shapefile, std::ifstream& sherrfile);
  void  SetTemplateMinMaxGoodADC(const Double_t min, const Double_t max); // This is used in making templates only
  void  SetTemplateMinMaxFitRange(const Double_t min, const Double_t max); // This is used in making templates only

  //Double_t FitPulse();
  void     SetTimeOffset(const Double_t o) { f_time_offset = o; }
  Double_t TemplateFcn(Double_t *x, Double_t *par);
  TF1*     GetTemplateFcn() { return template_fcn; }

  void PadUpdate();
  void Print();

private:
  int ch;
  int nsamples;

  /** fit values*/
  Double_t f_ampl;  /** from fit of spline or template */
  Double_t f_time;  /** from fit of spline or template */

  Double_t f_time_offset; /** time offset used in fit */
  
  Double_t f_integral;  /** integral */

  TH1 *hRawPulse;             //!
  TH1 *hSubPulse;             //!
  TH1 *hpulse;                //!
  TGraphErrors *gRawPulse;    //!
  TGraphErrors *gSubPulse;    //!
  TGraphErrors *gpulse;       //!

  /** for CalcPed0 */
  RunningStats *ped0stats;    //!
  TH1     *hPed0;             //!
  Double_t ped0;              //!
  Double_t ped0rms;           //!
  Int_t    use_ped0;          //! whether to apply ped0
  Int_t    minped0samp;       //! min sample for event-by-event ped, inclusive
  Int_t    maxped0samp;       //! max sample for event-by-event ped, inclusive
  Double_t minped0x;          //! min x for event-by-event ped, inclusive
  Double_t maxped0x;          //! max x for event-by-event ped, inclusive

  /** For pulse template extraction */
  TH2     *h2Template;
  TH2     *h2Residuals;
  //THnSparse *h2Template;
  TH1     *hAmpl;
  TH1     *hTime;
  Int_t    template_npointsx;
  Int_t    template_npointsy;
  Double_t template_begintime;
  Double_t template_endtime;
  Double_t template_min_good_amplitude;  //! for template, in original units of waveform data
  Double_t template_max_good_amplitude;  //! for template, in original units of waveform data
  Double_t template_min_xrange;          //! for template, in original units of waveform data
  Double_t template_max_xrange;          //! for template, in original units of waveform data
  std::vector<Double_t> template_y;
  std::vector<Double_t> template_yrms;
  TF1     *template_fcn;

  ClassDef(DigSig,1)
};

#endif  // __DIGSIG_H__
