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

void newfit_dt5742(const int nevents = 0, const int make_display=0)
{
  gROOT->ProcessLine(".L newdt5742.C");
  eventloop(nevents,make_display);
}

TH1 *hadc[MAXCH];
TH1 *htrace_flipped[MAXCH];   // Flipped waveform for TSpectrum peak finding
TProfile *hprof_adc[MAXCH]; // there seems to be a dependence of pedestal on sample number?
                            // after correcting , we get RMS = 2.0 ADC -> 1.8 ADC

const int NCH = 12; // 12 Argonne Channels

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

  TFile *savefile = new TFile("mcp_ana.root","RECREATE");
  TString name;
  TString title;
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
  t = new TTree("t","mcp-pmt data");
  t->Branch("evt",&f_evt,"evt/I");
  t->Branch("tstamp",&f_tstamp,"tstamp/I");
  t->Branch("spill",&f_spill,"spill/S");
  t->Branch("spillevt",&f_spillevt,"spillevt/S");
  t->Branch("ch",&f_ch,"ch[12]/S");
  t->Branch("ampl",&f_ampl,"ampl[12]/F");
  t->Branch("q",&f_q,"q[12]/F");
  t->Branch("t0",&f_t0,"t0[12]/F");
  t->Branch("t0a",&f_t0a,"t0a[12]/F");
  t->Branch("t0b",&f_t0b,"t0b[12]/F");
  t->Branch("t0c",&f_t0c,"t0c[12]/F");
  t->Branch("t0d",&f_t0d,"t0d[12]/F");
  t->Branch("v",f_volt,"v[12][1024]/F");    // waveform value in volts
  t->Branch("t",f_time,"t[12][1024]/F");    // waveform time in ns
  /*
  t->Branch("samp",&f_samp,"samp/S");
  t->Branch("adc",&f_adc,"adc/F");
  t->Branch("cadc",&f_cadc,"cadc/F");
  */

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
          trace[ich]->SetPoint(isamp,x,y-ped);

          f_time[finalch][isamp] = x;
          f_volt[finalch][isamp] = (y-ped)*gain[finalch];
        }

        for (int isamp=0; isamp<NSAMPLES; isamp++)
        {
          hprof_adc[ich]->Fill( isamp, adc[ich][isamp] );

          // Drop last 20 samples because they're always bad
          if ( isamp > (NSAMPLES-20) ) continue;

          hadc[ich]->Fill( adc[ich][isamp] );
          //f_samp = isamp;
          //f_adc = adc[ich][isamp];
          //f_cadc = adc[ich][isamp] - 3.518e-3*isamp; // what is this?

          /*
          double x, y;
          trace[ich]->GetPoint(isamp,x,y);
          htrace_flipped[ich]->Fill(x,y);
          */
        }

        // Get Simple amplitude and time
        double maxamp = -9999.;
        int    maxsamp = -9999;
        if ( verbose ) cout << ich << ":\t";
        getamplitude( trace[ich], maxamp, maxsamp );

        // Sanity check
        double y;
        double time0;
        trace[ich]->GetPoint(maxsamp,time0,y);
        if ( y != maxamp )
        {
          cout << "ERROR " << y << "\t" << maxamp << endl;
        }

        f_ampl[finalch] = -maxamp;
        if ( verbose ) cout << f_ampl[finalch];

        // Get simple time for this channel
        if ( do_display != 0 ) c_time->cd();

        float mintime = 10.;  // min of window where hit occurs
        float maxtime = 20.;  // max of window where hit occurs
//        float mintime = 160.;
//        float maxtime = 180.;
        f_t0[finalch] = gettimeCFD(trace[ich],0.2,25,mintime,maxtime);
        f_t0a[finalch] = gettime(trace[ich],maxamp,maxsamp);
        f_t0b[finalch] = gettime(trace[ich],-7,mintime,maxtime);
        f_t0c[finalch] = gettime(trace[ich],-21,mintime,maxtime);
        f_t0d[finalch] = gettime(trace[ich],-250,mintime,maxtime);

        if ( verbose ) cout << "\t" << f_t0[finalch];

        if ( do_display != 0 )
        {
          // Check the time
          trace[ich]->Draw("alp");
          trace[ich]->GetHistogram()->GetXaxis()->SetRangeUser(0.,60.);
          //trace[ich]->GetHistogram()->GetYaxis()->SetRangeUser(-30.,10.);
          //TVirtualPad::GetPad()->RedrawAxis();
          /*
             gPad->Modified();
             gPad->Update();
             */
          cout << "time?";
          string junk;
          cin >> junk;
          if ( junk[0] == 'q' )
          {
            gSystem->Exit(0);
          }
        }

        // Get simple charge for this channel
        // Q = \int{dQ} = \int{Vdt}/R
        f_q[finalch] = getintegral(trace[ich],10.0,25.0);
        f_q[finalch] = f_q[finalch]/50.;   // divide by R = 50 ohms
        f_q[finalch] = -f_q[finalch];   // convert to positive
        if ( verbose ) cout << "\t" << f_q[finalch];
        if ( verbose ) cout << endl;

        /*
        trace[ich]->Draw("alp");
        gPad->Modified();
        gPad->Update();
        string junk;
        cin >> junk;
        */

        // Find peaks using TSpectrum
        //tspectrum.Search(htrace_flipped[ich],2,"goff",0.08);
        /*
        tspectrum.Search(htrace_flipped[ich],2,"",0.08);
        int npeaks = tspectrum.GetNPeaks();
        Float_t *tdc_peaks = tspectrum.GetPositionX();
        htrace_flipped[ich]->Draw();
        gPad->Modified();
        gPad->Update();
        cout << "npeaks " << npeaks << endl;
        */

      }

      // Now draw the event
      for (int ich=0; ich<MAXCH; ich++)
      {
        // skip empty channels
        if ( ch2out[ich]<0 ) continue;

        if ( do_display != 0 )
        {
          int cid = ch2canvas[ich];
          canvas[ich/16]->cd( cid );
          name = "ch"; name += ich;
          trace[ich]->Draw("alp");
          trace[ich]->GetHistogram()->SetTitle(name);
          trace[ich]->GetHistogram()->SetAxisRange(0,100);
          trace[ich]->GetHistogram()->GetXaxis()->SetRangeUser(10.,60.);
          //gPad->Modified();
        }
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

