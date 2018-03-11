// read in ascii files from CAEN DT5742 wavedump output
//
#include <fstream>
#include <iostream>
#include <iomanip>
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TF1.h>
#include <TString.h>
#include <TPad.h>
#include <TCanvas.h>
#include "dt5742.h"

void dt5742_toroot(const int nevents = 0, const int make_display=0)
{
  gROOT->ProcessLine(".L dt5742.C");
  eventloop(nevents,make_display);
}

TH1 *hadc[MAXCH];
TH1 *htrace_flipped[MAXCH];   // Flipped waveform for TSpectrum peak finding
TProfile *hprof_adc[MAXCH]; // there seems to be a dependence of pedestal on sample number?
                            // after correcting , we get RMS = 2.0 ADC -> 1.8 ADC

const int NCH = 16; // 16 channels in DT5742
Double_t invert[NCH] = { -1., 1., 1., 1.,
                          1., 1., 1., 1.,
                          1., 1., 1., 1.,
                          1., 1., 1., 1. };

TTree *t;
Int_t    f_evt;
Int_t    f_tstamp;
Short_t  f_spill;
Short_t  f_spillevt;
Short_t  f_ch[NCH];    // Channel Number
Float_t  f_ampl[NCH];  // Amplitude
Float_t  f_q[NCH];     // Charge
Float_t  f_t0[NCH];    // Time-Zero
Float_t  f_t0a[NCH];    // Different Time-Zero
Float_t  f_t0b[NCH];    // Time-Zero
Float_t  f_t0c[NCH];    // Time-Zero
Float_t  f_t0d[NCH];    // Time-Zero
Float_t  f_time[NCH][NSAMPLES];  // waveform time
Float_t  f_volt[NCH][NSAMPLES];  // waveform voltage

Short_t  f_samp;
Float_t  f_adc;
Float_t  f_cadc;  // corrected adc


void eventloop(const int nevents = 0, const int do_display=0)
{
  int verbose = 0;

  // Channel to canvas map
  int ch2canvas[] = { 1, 3, 2, 4, 5, 6, -1, -1, 7, 8, 9, 10, 11, 12, -1, -1 };

  // Map FEM channel to output channel
  //int ch2out[] = { 0, 2, 1, 3, 4, 5, -1, -1, 6, 7, 8, 9, 10, 11, -1, -1 };
  //
  // For later run with channels swapped
  //int ch2out[] = { 0, 2, 1, 3, 4, 5, -1, -1, 6, 7, 8, 9, 10, 11, -1, -1 };
  //
  //For last runs, rate scan
  int ch2out[] = { 0, 1, 2, 3, 4, 5, -1, -1, 6, 7, 8, 9, 10, 11, -1, -1 };

  // gain from ADC to mV, in original CAEN channel
  float gain[NCH];
  for (int ich=0; ich<NCH; ich++)
  { 
    gain[ich] = 3.44e-4;
    //cout << ich << "\t" << gain[ich] << endl;
  }

  TFile *savefile = new TFile("dt5742.root","RECREATE");
  TString name;
  TString title;
  TString leaflist;
  for (int ich=0; ich<MAXCH; ich++)
    {
      name = "hadc"; name += ich;
      hadc[ich] = new TH1D(name,name,4096,-0.5,4095.5);
      hadc[ich]->SetXTitle("ADC Pedestal");

      name = "htrace_flipped"; name += ich;
      htrace_flipped[ich] = new TH1F(name,name,30*5,0.,30.);
      htrace_flipped[ich]->SetXTitle("t (ns)");

      name = "hprof_adc"; name += ich;
      hprof_adc[ich] = new TProfile(name,name,1024,-0.5,1023.5,-0.5,4095.5);
    }
  t = new TTree("t","DT5742 data");
  t->Branch("evt",&f_evt,"evt/I");
  t->Branch("tstamp",&f_tstamp,"tstamp/I");
  t->Branch("spill",&f_spill,"spill/S");
  t->Branch("spillevt",&f_spillevt,"spillevt/S");

  for (int ich=0; ich<NCH; ich++)
  {
    name = "ch"; name += ich;
    leaflist = name; leaflist += "[1024]/F";
    t->Branch(name,f_volt[ich],leaflist);    // waveform value in volts

    name = "t"; name += ich;
    leaflist = name; leaflist += "[1024]/F";
    t->Branch(name,f_time[ich],leaflist);    // waveform value in volts
  }

  OpenWaveFiles();

  TCanvas *canvas[NUM_PMT];
  if ( do_display != 0 )
  {
    for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
    {
      name = "c_mcp"; name += ipmt;
      title = "mcp"; title += ipmt;
      canvas[ipmt] = new TCanvas(name,title,1200,600);
      canvas[ipmt]->Divide(3,4);
    }
  }

  TSpectrum tspectrum;
  //TCanvas *c_spectrum = new TCanvas("c_spectrum","Peak Finder",550,425);
  TCanvas *c_time;
  if ( do_display != 0 )
  {
    c_time = new TCanvas("c_time","find time",550,425);
  }

  Int_t prev_tstamp = -1;
  f_spill = 1;

  for (int ievt=0; ievt<nevents || nevents==0; ievt++)
    {
      if ( ievt%10 == 0 ) cout << ievt << endl;
      int status = ReadSingleEvent();
      if ( status==0 ) break; // end of file

      f_evt = g_event_num + 1;
      if ( f_evt != (ievt+1) )
      {
        cout << "Error, event out of sequence " << g_event_num
         << " should be " << (ievt+1) << endl;
        break;
      }
      f_tstamp = g_time_stamp;
      cout << "Event " << f_evt << "\t" << g_event_num << "\t" << g_time_stamp << "\t" << g_time_stamp-prev_tstamp << endl;

      // check for end of spill
      if ( ievt>0 )
      {
        Int_t dt = f_tstamp - prev_tstamp;
        if ( dt<0 ) // had a rollover
        {
          dt = f_tstamp + 0x40000000 - prev_tstamp;
        }
        if ( dt > 20000000 )
        {
          f_spill++;
          cout << "new spill at " << f_evt << endl;
          cout << "Event " << f_evt << "\t" << g_event_num << "\t" << g_time_stamp << "\t" << g_time_stamp-prev_tstamp << endl;
        }
      }

      for (int ich=0; ich<MAXCH; ich++)
      {
        int finalch = ch2out[ich];       // final channel for output

        if ( finalch<0 ) continue;   // skip empty channels

        f_ch[finalch] = finalch;

        // Make pedestal subtracted traces
        float ped = getpedestal( trace[ich] );

        for (int isamp=0; isamp<NSAMPLES; isamp++)
        {
          double x, y;
          trace[ich]->GetPoint(isamp,x,y);

          //cout << ich << "\t" << isamp  << "\t" << x << "\t" << y << endl;
          f_time[finalch][isamp] = (Float_t)x;
          f_volt[finalch][isamp] = (Float_t)invert[ich]*y;
        }

      }

      // Now draw the event
      for (int ich=0; do_display!=0 && ich<MAXCH; ich++)
      {
        // skip empty channels
        if ( ch2out[ich]<0 ) continue;

        int cid = ch2canvas[ich];
        canvas[ich/16]->cd( cid );
        name = "ch"; name += ich;
        trace[ich]->Draw("alp");
        trace[ich]->GetHistogram()->SetTitle(name);
        trace[ich]->GetHistogram()->SetAxisRange(0,100);
        trace[ich]->GetHistogram()->GetXaxis()->SetRangeUser(10.,60.);
        //gPad->Modified();
      }

      if ( do_display != 0 )
      {
        for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
        {
          canvas[ipmt]->Modified();
          canvas[ipmt]->Update();

          /*
          string junk;
          cin >> junk;
          */
        }
      }

      /*
      name = "mcp0_evt"; name += ievt; name += ".png";
      canvas[0]->SaveAs( name );
      name = "mcp1_evt"; name += ievt; name += ".png";
      canvas[1]->SaveAs( name );
      */

      prev_tstamp = f_tstamp;

      // End of event processing, write out values to TTree
      t->Fill();
    }

  savefile->Write();
  CloseWaveFiles();

}

