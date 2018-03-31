#include "DigSig.h"
#include "RunningStats.h"

#include <TFile.h>
#include <TTree.h>
#include <TF1.h>
#include <TH2.h>
//#include <THnSparse.h>
#include <TSpline.h>
#include <TPad.h>
#include <TMath.h>
#include <TGraphErrors.h>
#include <TSpectrum.h>

#include <iostream>
#include <fstream>

using namespace std;

/*
DigSig::DigSig()
{
}
*/

DigSig::DigSig(const int chnum, const int nsamp) :
ch(chnum), nsamples(nsamp)
{
  //cout << "In DigSig::DigSig(" << ch << "," << nsamples << ")" << endl;
  TString name;

  ch = chnum;
  nsamples = nsamp;

  name = "hrawpulse"; name += ch;
  hRawPulse = new TH1F(name,name,nsamples,-0.5,nsamples-0.5);
  name = "hsubpulse"; name += ch;
  hSubPulse = new TH1F(name,name,nsamples,-0.5,nsamples-0.5);

  gRawPulse = new TGraphErrors(nsamples);
  name = "grawpulse"; name += ch;
  gRawPulse->SetName(name);
  gSubPulse = new TGraphErrors(nsamples);
  name = "gsubpulse"; name += ch;
  gSubPulse->SetName(name);

  hpulse = hRawPulse;   // hpulse,gpulse point to raw by default
  gpulse = gRawPulse;   // we switch to sub for default if ped is applied

  ped0stats = new RunningStats();
  ped0 = 0.;  // ped average
  use_ped0 = 0;
  name = "hPed0_"; name += ch;
  //hPed0 = new TH1F(name,name,16384,-0.5,16383.5);
  hPed0 = new TH1F(name,name,1000,1,0); // automatically determine the range
  minped0samp = -9999;
  maxped0samp = -9999;
  minped0x = 0.;
  maxped0x = 0.;

  h2Template = 0;
  h2Residuals = 0;
  SetTemplateSize(120,2048,-2,9.9);

  // range of good amplitudes for templates
  // units are usually in ADC counts
  template_min_good_amplitude = 20.;
  template_max_good_amplitude = 4080.;

  // time shift from fit
  f_time_offset = 4.0;

  name = "hAmpl"; name += ch;
  hAmpl = new TH1F(name,name,17100,-100,17000);
  name = "hTime"; name += ch;
  hTime = new TH1F(name,name,3100,0,31);

  template_fcn = 0;
}

void  DigSig::SetTemplateSize(const Int_t nptsx, const Int_t nptsy, const Double_t begt, const Double_t endt)
{
  template_npointsx = nptsx;
  template_npointsy = nptsy;
  template_begintime = begt;
  template_endtime = endt;

  template_y.resize(template_npointsx);
  template_yrms.resize(template_npointsx);

  Double_t xbinwid = (template_endtime - template_begintime)/(template_npointsx-1);
  Double_t ybinwid = (1.1+0.1)/template_npointsy;  // yscale... should we vary this?
  if ( h2Template != 0 ) delete h2Template;
  if ( h2Residuals != 0 ) delete h2Residuals;

  TString name = "h2Template"; name += ch;
  h2Template = new TH2F(name,name,template_npointsx,template_begintime-xbinwid/2.,template_endtime+xbinwid/2,
      template_npointsy,-0.1+ybinwid/2.0,1.1+ybinwid/2.0);
 
  name = "h2Residuals"; name += ch;
  h2Residuals = new TH2F(name,name,template_npointsx,template_begintime-xbinwid/2.,template_endtime+xbinwid/2,
      80,-20,20);
 
  /*
  int nbins[] = { template_npointsx, nbinsy };
  Double_t lowrange[] = { template_begintime-xbinwid/2.0, -0.1+ybinwid/2.0 };
  Double_t highrange[] = { template_endtime+xbinwid/2.0, 1.1+ybinwid/2.0 };
  h2Template = new THnSparseF(name,name,2,nbins,lowrange,highrange);
  */
  //h2Template->cd( gDirectory );

}

DigSig::~DigSig()
{
  /*
  delete hRawPulse;
  delete hSubPulse;
  delete gRawPulse;
  delete gSubPulse;
  delete ped0stats;
  */
  //h2Template->Write();
}

void  DigSig::SetTemplateMinMaxGoodADC(const Double_t min, const Double_t max)
{
  template_min_good_amplitude = min;
  template_max_good_amplitude = max;
}

void  DigSig::SetTemplateMinMaxFitRange(const Double_t min, const Double_t max)
{
  template_min_xrange = min;
  template_max_xrange = max;
}

// This sets y, and x to sample number (starts at 0)
void DigSig::SetY(const Float_t *y, const int invert)
{
  hpulse->Reset();

  for (int isamp=0; isamp<nsamples; isamp++)
  {
    hRawPulse->SetBinContent( isamp+1, y[isamp] );
    gRawPulse->SetPoint( isamp, Double_t(isamp), y[isamp] );
  }

  // Apply pedestal
  if ( use_ped0 != 0 || minped0samp >= 0 || minped0x != maxped0x )
  {
    //cout << "sub" << endl;

    if ( minped0samp >= 0 )
    {
      CalcEventPed0(minped0samp,maxped0samp);
    }
    else if ( minped0x != maxped0x )
    {
      CalcEventPed0(minped0x,maxped0x);
    }

    for (int isamp=0; isamp<nsamples; isamp++)
    {
      hSubPulse->SetBinContent( isamp+1, invert*(y[isamp]-ped0) );
      hSubPulse->SetBinError( isamp+1, ped0rms );
      gSubPulse->SetPoint( isamp, (Double_t)isamp, invert*(y[isamp]-ped0) );
      gSubPulse->SetPointError( isamp, 0., ped0rms );
    }
  }
}

void DigSig::SetXY(const Float_t *x, const Float_t *y, const int invert)
{
  hRawPulse->Reset();
  hSubPulse->Reset();
  //cout << nsamples << endl;
 
  //cout << "use_ped0 " << use_ped0 << "\t" << ped0 << endl;

  for (int isamp=0; isamp<nsamples; isamp++)
  {
    //cout << isamp << "\t" << x[isamp] << "\t" << y[isamp] << endl;
    hRawPulse->SetBinContent( isamp+1, y[isamp] );
    gRawPulse->SetPoint( isamp, x[isamp], y[isamp] );
  }

  if ( use_ped0 != 0 || minped0samp >= 0 || minped0x != maxped0x )
  {
    if ( minped0samp >= 0 )
    {
      CalcEventPed0(minped0samp,maxped0samp);
    }
    else if ( minped0x != maxped0x )
    {
      CalcEventPed0(minped0x,maxped0x);
    }

    for (int isamp=0; isamp<nsamples; isamp++)
    {
      {
        //cout << "sub" << endl;
        // How do we handle data which is not in samples, but is in time,
        // such as DRS4 data
        hSubPulse->SetBinContent( isamp+1, invert*(y[isamp]-ped0) );
        hSubPulse->SetBinError( isamp+1, ped0rms );
        gSubPulse->SetPoint( isamp, x[isamp], invert*(y[isamp]-ped0) );
        gSubPulse->SetPointError( isamp, 0., ped0rms );
      }
    }
  }
}

// This does a straight line fit for now...
/*
Double_t DigSig::FitPulse()
{
  const Double_t pedcut[] = {1650,1560};
  const Double_t maxcut[] = {12000,12600};

  TF1 f("f","pol1",0,31);
  f.SetParameter(0,0);
  f.SetParameter(1,3000.);

  Double_t start = 0;
  Double_t stop = 31;
  Double_t x, y;
  for (int isamp=0; isamp<nsamples; isamp++)
  {
    gpulse->GetPoint(isamp,x,y);
    if ( y>pedcut[ch] )
    {
      start = x;
      break;
    }
  }
  
  for (int isamp=(int)start; isamp<nsamples; isamp++)
  {
    gpulse->GetPoint(isamp,x,y);
    if ( y>maxcut[ch] )
    {
      stop = x-1;
      break;
    }
  }

  f.SetRange(start,stop);
  gpulse->Fit(&f,"R");

  Double_t slope = f.GetParameter(1);

  //cout << "xxx " << slope << endl;
  return slope;
}
*/

void DigSig::FillPed0(const Int_t sampmin, const Int_t sampmax)
{
  Double_t x, y;
  for (int isamp=sampmin; isamp<=sampmax; isamp++)
  {
    gRawPulse->GetPoint(isamp,x,y);
    hPed0->Fill( y );

    ped0stats->Push( y );
    ped0 = ped0stats->Mean();
    ped0rms = ped0stats->RMS();
    //cout << "ped0 " << ch << " " << n << "\t" << ped0 << endl;
  }

}


void DigSig::FillPed0(const Double_t begin, const Double_t end)
{
  Double_t x, y;
  Int_t n = gRawPulse->GetN();
  for (int isamp=0; isamp<n; isamp++)
  {
    gRawPulse->GetPoint(isamp,x,y);
    if ( x>=begin && x<=end )
    {
      hPed0->Fill( y );

      ped0stats->Push( y );
      ped0 = ped0stats->Mean();
      ped0rms = ped0stats->RMS();
      //cout << "ped0 " << ch << " " << n << "\t" << x << "\t" << y << endl;
    }

    // quit if we are past the ped region
    if ( x>end ) break;
  }

}


void DigSig::SetPed0(const Double_t mean, const Double_t rms)
{
  ped0 = mean;
  ped0rms = rms;
  use_ped0 = 1;
  hpulse = hSubPulse;
  gpulse = gSubPulse;
  //cout << "ch " << ch << " Ped = " << ped0 << endl;
}

// Get Event by Event Ped9 if requested
void DigSig::CalcEventPed0(const Int_t minpedsamp, const Int_t maxpedsamp)
{
  hPed0->Reset();
  ped0stats->Clear();

  Double_t x, y;
  for (int isamp=minpedsamp; isamp<=maxpedsamp; isamp++)
  {
    gRawPulse->GetPoint(isamp,x,y);

    hPed0->Fill(y);
    ped0stats->Push( y );
  }

  // use straight mean for pedestal
  // Could consider using fit to hPed0 to remove outliers
  SetPed0( ped0stats->Mean(), ped0stats->RMS() );
}

// Get Event by Event Ped0 if requested
void DigSig::CalcEventPed0(const Double_t minpedx, const Double_t maxpedx)
{
  hPed0->Reset();
  ped0stats->Clear();

  Double_t x, y;
  Int_t n = gRawPulse->GetN();

  for (int isamp=0; isamp<n; isamp++)
  {
    gRawPulse->GetPoint(isamp,x,y);

    if ( x>= minpedx && x<= maxpedx)
    {
      hPed0->Fill(y);
      ped0stats->Push( y );
    }
  }

  // use straight mean for pedestal
  // Could consider using fit to hPed0 to remove outliers
  Double_t mean = ped0stats->Mean();
  Double_t rms = ped0stats->RMS();
  SetPed0( mean, rms );
  //cout << "ped0stats " << mean << "\t" << rms << endl;
}

Double_t DigSig::LeadingEdge(const Double_t threshold)
{
  // Find first point above threshold
  // We also make sure the next point is above threshold
  // to get rid of a high fluctuation
  int n = gSubPulse->GetN();
  Double_t *x = gSubPulse->GetX();
  Double_t *y = gSubPulse->GetY();

  int sample = -1;
  for (int isamp=0; isamp<n; isamp++)
  {
    if ( y[isamp] > threshold )
    {
      if ( isamp==n || y[isamp+1] > threshold )
      {
        sample = isamp;
        break;
      }
    }
  }
  if ( sample == -1 ) return -9999.;  // no signal above threshold

  // Linear Interpolation of start time
  Double_t dx = x[sample] - x[sample-1];
  Double_t dy = y[sample] - y[sample-1];
  Double_t dt1 = y[sample] - threshold;

  Double_t t0 = x[sample] - dt1*(dx/dy);

  return t0;
}

Double_t DigSig::dCFD(const Double_t fraction_threshold)
{
  // Find first point above threshold
  // We also make sure the next point is above threshold
  // to get rid of a high fluctuation
  int n = gSubPulse->GetN();
  Double_t *x = gSubPulse->GetX();
  Double_t *y = gSubPulse->GetY();

  // Get max amplitude
  Double_t ymax = TMath::MaxElement(n,y);

  Double_t threshold = fraction_threshold * ymax; // get fraction of amplitude
  //cout << "threshold = " << threshold << "\tymax = " << ymax <<endl;

  int sample = -1;
  for (int isamp=0; isamp<n; isamp++)
  {
    if ( y[isamp] > threshold )
    {
      if ( isamp==n || y[isamp+1] > threshold )
      {
        sample = isamp;
        break;
      }
    }
  }
  if ( sample == -1 ) return -9999.;  // no signal above threshold

  // Linear Interpolation of start time
  Double_t dx = x[sample] - x[sample-1];
  Double_t dy = y[sample] - y[sample-1];
  Double_t dt1 = y[sample] - threshold;

  Double_t t0 = x[sample] - dt1*(dx/dy);

  return t0;
}

Double_t DigSig::Integral(const Double_t xmin, const Double_t xmax)
{
  Int_t n = gSubPulse->GetN();
  Double_t* x = gSubPulse->GetX();
  Double_t* y = gSubPulse->GetY();

  f_integral = 0.;
  for (int ix=0; ix<n; ix++)
  {
    if (x[ix]>=xmin && x[ix]<=xmax)
    {
      // Get dx
      Double_t dx = (x[ix+1]-x[ix-1])/2.0;
      f_integral += (y[ix]*dx);
    }
  }

  return f_integral;
}

void DigSig::LocMax(Double_t& x_at_max, Double_t& ymax)
{
  // Find index of maximum peak
  Int_t n = gSubPulse->GetN();
  Double_t* x = gSubPulse->GetX();
  Double_t* y = gSubPulse->GetY();
  int locmax = TMath::LocMax(n,y);
  x_at_max = x[locmax];
  ymax = y[locmax];
}

void DigSig::LocMin(Double_t& x_at_max, Double_t& ymax)
{
  // Find index of maximum peak
  Int_t n = gSubPulse->GetN();
  Double_t* x = gSubPulse->GetX();
  Double_t* y = gSubPulse->GetY();
  int locmax = TMath::LocMin(n,y);
  x_at_max = x[locmax];
  ymax = y[locmax];
}

void DigSig::Print()
{
  Double_t x, y;
  cout << "CH " << ch << endl;
  for (int isamp=0; isamp<nsamples; isamp++)
  {
    gpulse->GetPoint(isamp,x,y);
    cout << isamp << "\t" << x << "\t" << y << endl;
  }
}

void DigSig::PadUpdate()
{
  // Make sure TCanvas is created externally!
  gPad->Modified();
  gPad->Update();
  cout << ch << " ? ";
  TString junk;
  cin >> junk;
}

Double_t DigSig::TemplateFcn(Double_t *x, Double_t *par)
{
  // par[0] is the amplitude (relative to the spline amplitude)
  // par[1] is the start time (in sample number)
  // x[0] units are in sample number
  Double_t xx = x[0]-par[1];
  Double_t f = 0.;

  int verbose = 0;
  //verbose = 100;

  // When fit is out of limits of good part of spline, ignore fit
  if ( xx<template_begintime || xx>template_endtime )
    {
      TF1::RejectPoint();
      if ( xx < template_begintime )
        {
          //Double_t x0,y0;
          Double_t y0 = template_y[0];
          return par[0]*y0;
        }
      else if ( xx > template_endtime )
        {
          //Double_t x0,y0;
          Double_t y0 = template_y[template_npointsx-1];
          return par[0]*y0;
        }
    }

  // Linear Interpolation of template
  Double_t x0 = 0.;
  Double_t y0 = 0.;
  Double_t x1 = 0.;
  Double_t y1 = 0.;

  // find the index in the vector which is closest to xx
  Double_t step = (template_endtime - template_begintime) / (template_npointsx-1);
  Double_t index = (xx - template_begintime)/step;

  int ilow = TMath::FloorNint( index );
  int ihigh = TMath::CeilNint( index );
  if ( ilow < 0 || ihigh >= template_npointsx )
    {
      if ( verbose>0 )
      {
        cout << "ERROR, ilow ihigh " << ilow << "\t" << ihigh << endl;
        cout << " " << xx << " " << x[0] << " " << par[1] << endl;
      }

      if ( ilow<0 )
      {
        ilow = 0;
      }
      else if ( ihigh >= template_npointsx )
      {
        ihigh = template_npointsx - 1;
      }
    }

  if ( ilow==ihigh )
    {
      f = par[0]*template_y[ilow];
    }
  else
    {
      x0 = template_begintime + ilow*step;
      y0 = template_y[ilow];
      x1 = template_begintime + ihigh*step;
      y1 = template_y[ihigh];
      f = par[0]*(y0+((y1-y0)/(x1-x0))*(xx-x0));  // linear interpolation
    }

  return f;
}

int DigSig::FitTemplate()
{
  int verbose = 0;
  //verbose = 100;	// uncomment to see fits
  if ( verbose>0 ) cout << "Fitting ch " << ch << endl;
 
  /*
  Int_t maxadc = -1;
  Int_t peak_samp = -1;
  for (int isamp=0; isamp<NSAMPLES; isamp++)
    {
      x[isamp] = (Float_t)isamp;
      adcsub[isamp] = adc[isamp] - ped;
      adcerr[isamp] = pedrms;
      if ( adcsub[isamp]>maxadc )
        {
          maxadc = adcsub[isamp];
          peak_samp = isamp;
        }
    }

  TGraphErrors gSubpulse(NSAMPLES,x,adcsub,0,adcerr);
  if ( verbose>10 )
    {
      gSubPulse->SetMarkerStyle(20);
      gSubPulse->SetMarkerColor(2);
      gSubPulse->SetLineColor(2);
    }

  fit_shape = fit_pshape[ch];
  fit_sherr = fit_psherr[ch];
  //if ( verbose>10 ) cout << "CH is " << ch << "\t" << fit_shape << endl;
  */
  
  // Get x-position of maximum
  Double_t x_at_max, ymax;
  LocMax(x_at_max, ymax);

  template_fcn->SetParameters(ymax, x_at_max);

  //template_fcn->SetParLimits(1,-5.,4.);
  template_fcn->SetRange(template_min_xrange,template_max_xrange);
  gSubPulse->Fit(template_fcn,"RNQ");
  //gSubPulse->Fit(template_fcn,"R");

  // Get fit parameters
  f_ampl = template_fcn->GetParameter(0);
  f_time = template_fcn->GetParameter(1);

  if ( verbose>0 && fabs(f_ampl) > 0. )
  {
    cout << "FitTemplate " << f_ampl << "\t" << f_time << endl;
    gSubPulse->Draw("ap");
    template_fcn->SetLineColor(4);
    template_fcn->Draw("same");
    PadUpdate();
  }

  /*
  Double_t chi2ndf = template_fcn->GetChisquare()/template_fcn->GetNDF();
  // Store fit values
  amp = static_cast<Float_t>( template_fcn->GetParameter(0) );
  fquality = (Short_t)chi2ndf;	// Need to define this still

  // For the tdc, we choose 360 tdc ticks per sample
  Float_t samp_number = 7.0+template_fcn->GetParameter(1);
  if ( samp_number<0. ) 
    {
      samp_number = 0.;
      amp = 0;    // for now, if the time is bad, we zero out the channel
    }
  else if ( samp_number>18.0 )
    {
      samp_number = 18.0;
      amp = 0;
    }
  tdc = static_cast<Short_t>( samp_number*360. );
  */

  return 1;
}



int DigSig::FillSplineTemplate()
{
  int verbose = 0;
  //verbose = 100;

  Double_t max = TMath::MaxElement(gSubPulse->GetN(),gSubPulse->GetY());

  if ( max < template_min_good_amplitude ) return 0;

  if ( verbose ) gSubPulse->Draw("ap");
  TSpline3 s3("s3",gSubPulse);

  // First find maximum, to rescale
  f_ampl = -999999.;
  double step_size = h2Template->GetXaxis()->GetBinWidth(1);
  //cout << "step size " << step_size << endl;
  for (double ix=0; ix<nsamples; ix += step_size)
  {
    Double_t val = s3.Eval(ix);
    if ( val > f_ampl )
    {
      f_ampl = val;
    }
  }

  //cout << f_ampl << endl;
  if ( f_ampl<template_min_good_amplitude || f_ampl>template_max_good_amplitude ) return 0; 

  if ( verbose>0 )
  {
    //cout << ch << endl;
    s3.SetLineColor(2);
    s3.Draw("same");
    gSubPulse->Draw("p");
    PadUpdate();
  }


  // Now go back to find the time by finding x at midpoint of rise
  for (double ix=0; ix<nsamples; ix += step_size)
  {
    Double_t val = s3.Eval(ix);
    if ( val > 0.5*f_ampl )
    {
      // interpolate midpoint
      Double_t dy_mid = val - 0.5*f_ampl;
      Double_t dy_prev = val - s3.Eval(ix-step_size);
      Double_t dx_prev = step_size;

      f_time = ix - (dy_mid/dy_prev)*dx_prev;
      break;
    }
  }

  // correct the pulse back
  f_time -= f_time_offset;

  // Get Time and Max of spline to rescale
  //cout << f_ampl << "\t" << time << endl;
  hAmpl->Fill( f_ampl );
  hTime->Fill( f_time );

  //cout << "nsamples " << nsamples << endl;
  for (int isamp=0; isamp<nsamples; isamp++)
  {
    Double_t x, y;
    gSubPulse->GetPoint(isamp,x,y);

    Double_t fillvalues[2] = {0.};
    fillvalues[0] = x - f_time; //corr_time
    if ( f_ampl != 0. )
    {
      fillvalues[1] = y/f_ampl; //scaled_ampl
    }

    h2Template->Fill(fillvalues[0],fillvalues[1]);
    //h2Template->Fill( fillvalues );

    // For splines, by defn they go through the data pts so residuals = 0
    //h2Residuals->Fill( fillvalues[0], y - s3.Eval(x) );
  }

  return 1;
}


void DigSig::MakeAndWriteTemplate(ostream& out, ostream& oerr)
{
  int verbose = 0;
  //verbose = 100;
  if ( verbose ) cout << "In  DigSig::MakeAndWriteTemplate" << endl;

  //Int_t nbinsy = h2Template->GetNbinsX();
 
  TString name = h2Template->GetName(); name += "_py";
  TH1 *hprojy = h2Template->ProjectionY(name,1,1,"e");
  TF1 *gaus = new TF1("gaussian","gaus",template_begintime,template_endtime);
  gaus->SetLineColor(2);

  if ( verbose )
  {
    h2Template->Draw("colz");
    //TH2 *h = h2Template->Projection(1,0);
    //h->Draw("colz");
    PadUpdate();
    //delete h;
  }

  TString fitarg = "R";
  if ( verbose==0 ) fitarg += "NQ";

  for (int ibin=0; ibin<template_npointsx; ibin++)
  {
    hprojy = h2Template->ProjectionY(name,ibin+1,ibin+1,"e");
   
    //h2Template->GetAxis(0)->SetRange(ibin+1,ibin+1);
    //TH1 *hprojy = h2Template->Projection(1,"e");
    //hprojy->Sumw2();

    Double_t ncounts = hprojy->Integral();
    if ( ncounts < 10. )
    {
      cout << "ERROR, " << ch << "\t" << ibin << "\tToo few entries " << hprojy->Integral() << endl;
      fitarg += "WLL";
    }
    else if ( ncounts<100. )
    {
      fitarg += "WLL";
    }

    Double_t peak = hprojy->GetBinContent( hprojy->GetMaximumBin() );
    Double_t mean = hprojy->GetBinCenter( hprojy->GetMaximumBin() );
    Double_t rms = hprojy->GetRMS();
    //cout << ch << "\t" << ibin << "\t" << peak << "\t" << mean << endl;
    //gaus->SetParameters(peak,mean,0.01);
    gaus->SetParameters(peak,mean,rms);

    hprojy->Fit(gaus,fitarg);

    // Uncomment to see the fits
    if ( verbose )
    {
      hprojy->Draw();
      PadUpdate();
    }

    /*
    template_y[ibin] = gaus->GetParameter(1);
    template_yrms[ibin] = gaus->GetParameter(2);
    */

    template_y[ibin] = hprojy->GetMean();
    template_yrms[ibin] = hprojy->GetRMS();

    if ( verbose ) cout << "ibin " << ibin << "\t" << template_y[ibin] << "\t" << template_yrms[ibin] << endl;
    //delete hprojy;

  }
  delete hprojy;
  delete gaus;

  /* //thnsparse
  h2Template->GetAxis(0)->SetRange(1,template_npointsx);
  h2Template->Write();
  */

  out << ch << "\t" << template_npointsx << "\t" << template_begintime << "\t" << template_endtime << endl;
  oerr << ch << "\t" << template_npointsx << "\t" << template_begintime << "\t" << template_endtime << endl;
  for (int ibin=0; ibin<template_npointsx; ibin++)
  {
    // Write out the template value for bin i
    out << template_y[ibin] << "\t";
    if (ibin%10==9) out << endl;

    // Now write out the rms
    oerr << template_yrms[ibin] << "\t";
    if (ibin%10==9) oerr << endl;
  }
  out << endl;
  oerr << endl;

}

void DigSig::FillFcnTemplate()
{
  int verbose = 0;
  verbose = 100;

  if ( ch==0 || ch==2 || ch==3 )
  {
  FitTemplate();
  }
  else
  {
    f_ampl = 1.;
    f_time = 100.;
  }
  if ( f_ampl<template_min_good_amplitude || f_ampl>template_max_good_amplitude ) return; 

  // Get Time and Max of spline to rescale
  if ( verbose ) cout << "ampl time " << ch << "\t" << f_ampl << "\t" << f_time << endl;
  hAmpl->Fill( f_ampl );
  hTime->Fill( f_time );

  // Rescale and shift time using template fit
  for (int isamp=0; isamp<nsamples; isamp++)
  {
    Double_t x, y;
    gSubPulse->GetPoint(isamp,x,y);

    Double_t fillvalues[2] = {0};
    fillvalues[0] = x - f_time; //corr_time
    //Double_t scaled_ampl = 0.;
    if ( f_ampl != 0 )
    {
      fillvalues[1] = y/f_ampl; //scaled_ampl
    }
    h2Template->Fill( fillvalues[0], fillvalues[1] );
    //h2Template->Fill( fillvalues );
  }
}


int DigSig::ReadTemplate(ifstream& shapefile, ifstream& sherrfile)
{
  int verbose = 0;
  verbose = 100;
  Int_t temp_ch = -9999;
  Int_t temp_nsamples;
  Double_t temp_begintime;
  Double_t temp_endtime;

  // Template 
  while ( shapefile >> temp_ch >> temp_nsamples >> temp_begintime >> temp_endtime )
    {
      if ( verbose ) cout << "shape " << temp_ch << "\t" <<  temp_nsamples << "\t" <<  temp_begintime << "\t" <<  temp_endtime << endl;
      if ( temp_ch != ch )
        {
          cerr << "ERROR in shape: ch is " << temp_ch << "but should be " << ch << endl;
          return -1;
        }

      Double_t temp_val;
      for (int isamp=0; isamp<temp_nsamples; isamp++)
        {
          shapefile >> temp_val;
          template_y[isamp] = temp_val;
          if ( verbose )
          {
            cout << temp_val << " ";
            if ( isamp%10==9 ) cout << endl;
          }
        }
      if ( verbose ) cout << endl;
      break;
    }

  // Now get the errors
  while ( sherrfile >> temp_ch >> temp_nsamples >> temp_begintime >> temp_endtime )
    {
      if ( verbose ) cout << "sherr " << temp_ch << "\t" <<  temp_nsamples << "\t" <<  temp_begintime << "\t" <<  temp_endtime << endl;
      if ( temp_ch != ch )
        {
          cerr << "ERROR in sherr: ch is " << temp_ch << " but should be " << ch << endl;
          return -1;
        }

      Double_t temp_val;
      for (int isamp=0; isamp<temp_nsamples; isamp++)
        {
          sherrfile >> temp_val;
          template_yrms[isamp] = temp_val;
          if ( verbose )
          {
            cout << temp_val << " ";
            if ( isamp%10==9 ) cout << endl;
          }
        }
      if ( verbose ) cout << endl;
      break;
    }

  TString name = "template_fcn"; name += ch;
  template_fcn = new TF1(name,this,&DigSig::TemplateFcn,0,nsamples,2,"DigSig","TemplateFcn");
  template_fcn->SetParameters(1,8);

  return 1;
}

