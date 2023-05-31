#include <iostream>

#if defined(__CLING__)
R__LOAD_LIBRARY(libdigsig)
#endif

/*
//NEW BBC Electronics
const int NCH = 2;
const int NSAMP = 31;
//TString template_fname = "sig_gen2";
TString template_fname = "TIMING16_TEMPLATE/sig_gen2";
*/

//MBD DS TEST
const int MAXCH = 256;
const int NPMT = 128;
//const int NSAMP = 20;
const int NSAMP = 31;
int NCH = MAXCH;    // number of ch's to process

TString template_fname = "TEMPLATES/sig_gen0";
//TString template_fname = "TEMPLATES/sig_gen2";

int verbose = 0;

Int_t   f_evt;
Short_t f_bn[2];  // number of hit PMTs, [arm]
Float_t f_bq[2];  // Total charge sum, [arm]
Float_t f_bt[2];  // Mean Time, [arm]
Float_t f_bz;   // BBC zvtx
Float_t f_bt0;  // BBC t0
Float_t f_tt[NPMT];  // time from t-channel
Float_t f_tq[NPMT];  // time from q-channel
Float_t f_q[NPMT];   // charge

int time_method[MAXCH];
//const int TRIG_SAMP = 6;     // use this sample to get the MBD time
//const int TRIG_SAMP = 9;     // use this sample to get the MBD time
//const int TRIG_SAMP = 9;     // use this sample to get the MBD time
//const int TRIG_SAMP = 14;     // use this sample to get the MBD time
//const int TRIG_SAMP = 5;     // use this sample to get the MBD time
//const int TRIG_SAMP = 11;     // use this sample to get the MBD time
const int TRIG_SAMP = 17;     // use this sample to get the MBD time
//const int TRIG_SAMP = 10;     // use this sample to get the MBD time

// 0 = dCFD, 1=template fit, 2=MBD-method
void set_time_method()
{
  for (int ich=0; ich<NCH; ich++)
  {
    int tq = (ich/8)%2;   // 0=time ch, 1=charge ch

    if ( tq==0 )     time_method[ich] = 2;  // time ch
    else             time_method[ich] = 0;  // charge ch
  }
}

// Read in tdc2ns calibration results
TF1 *tdc2time[MAXCH];   // converts tdc to time
void read_tcalib(const char *tcalibfname = "mbd.tcalib")
{
  cout << "Reading TDC 2 Time(ns) calibrations from " << tcalibfname << endl;
  ifstream tcalibfile( tcalibfname );

  for (int ich=0; ich<NCH; ich++)
  {
    // skip charge channels
    if ( (ich/8)%2 == 1 ) continue;

    Int_t ch;
    Double_t p0, p1, p2, p3;
    Double_t p0err, p1err, p2err, p3err;
    tcalibfile >> ch >> p0 >> p0err >> p1 >> p1err >> p2 >> p2err >> p3 >> p3err;
    cout << ch << "\t" << p0 << "\t" << p0err << "\t" << p1 << "\t" << p1err << "\t" << p2 << "\t" << p2err
      << "\t" << p3 << "\t" << p3err << endl;

    tdc2time[ch] = new TF1("name","pol3",0,30);
    tdc2time[ch]->SetParameters(p0,p1,p2,p3);
  }
}

void reset_event()
{
  for (int iarm=0; iarm<2; iarm++)
  {
    f_bn[iarm] = 0;       // number of hit PMTs, north
    f_bq[iarm] = 0.;      // Total charge sum, north
    f_bt[iarm] = -9999.;  // Mean Time, north
    f_bz = -9999.;        // BBC zvtx
    f_bt0 = -9999.;       // BBC t0
  }
}

int digsig_calc_mbd(const char *rootfname = "calib_mbd-00008526-0000.root", const int nevents = 0)
{
  gSystem->Load("libdigsig.so");
  //Float_t f_evt;

  // Get the number of actual channels to process
  NCH = MAXCH;                 // total number of channels (including charge channels)

  ifstream configfile("digsig.cfg");
  if ( configfile.is_open() )
  {
    string junk;
    configfile >> junk >> NCH;

    cout << "Found config file digsig.cfg" << endl;
    cout << "Setting NCH = " << NCH << endl;
  }

//chiu SKIP FOR NOW
  //read_tcalib();

  TString savefname = rootfname;
  savefname.ReplaceAll(".root","_times.root");
  TFile *savefile = new TFile(savefname,"RECREATE");

  TString name, leaf;
  TTree *tree = new TTree("t","DigSig times");
  tree->Branch("evt", &f_evt, "evt/I");
  tree->Branch("bns", &f_bn[0], "bns/S");
  tree->Branch("bnn", &f_bn[1], "bnn/S");
  tree->Branch("bqs", &f_bq[0], "bqs/F");
  tree->Branch("bqn", &f_bq[1], "bqn/F");
  tree->Branch("bts", &f_bt[0], "bts/F");
  tree->Branch("btn", &f_bt[1], "btn/F");
  tree->Branch("bz",  &f_bz,  "bz/F");
  tree->Branch("bt0", &f_bt0, "bt0/F");
  for (int ipmt=0; ipmt<NPMT; ipmt++)
  {
    name = "tt"; name += ipmt; 
    leaf = name; leaf += "/F";
    tree->Branch(name, &f_tt[ipmt], leaf);
    name = "tq"; name += ipmt; 
    leaf = name; leaf += "/F";
    tree->Branch(name, &f_tq[ipmt], leaf);
    name = "q"; name += ipmt; 
    leaf = name; leaf += "/F";
    tree->Branch(name, &f_q[ipmt], leaf);
  }

  TCanvas *ac = new TCanvas("ac","ac",800,800);
  ac->Divide(4,4);

  DigAna digana(NCH,NSAMP);

  // Enable only certain channels for analysis
  /*
  digana.SkipAll();
  digana.EnableCh(4);
  digana.EnableCh(6);
  digana.EnableCh(12);
  digana.EnableCh(14);
  */

  // Set Time Reco Method to use
  set_time_method();

  int nentries = digana.OpenRootFile( rootfname );
  if ( nevents != 0 ) nentries = nevents;

  // Set Calibrations

  // Do evt-by-evt pedestal using sample range below
  digana.SetEventPed0Range(0,1);

//chiu SKIP FOR NOW
/*
  TString pedfname = rootfname;
  pedfname.ReplaceAll(".root","_ped.txt");
  Int_t index = pedfname.Last('/');
  if ( index>=0 ) pedfname.Remove(0,index+1);
  pedfname.Prepend("PED/");
  int pedstatus = digana.SetPed0FromFile( pedfname );
  if ( pedstatus<0 )
  {
    cerr << "ERROR: can't open file " << pedfname << "\t" << pedstatus << endl;
    return 0;
  }
*/

  // Now read in template waveforms
  // Must change to be channel specific!!!!
  if ( time_method[0]==1 )
  {
    digana.SetTemplateSize(600,8000,-15,15);
    digana.ReadTemplate(template_fname);
  }

  string junk;
  Double_t ampl, time;
  TLine line[MAXCH];

  for (int ievt=0; ievt<nentries; ievt++)
  {
    if ( ievt%1000 == 0 ) cout << "Evt " << ievt << endl;
    digana.ProcessEvent(ievt);
    f_evt = digana.get_evt();

    reset_event();

    for (int ich=0; ich<NCH; ich++)
    {
      DigSig *sig = digana.GetSig(ich);

      int sn = ich/128;     // south or north
      //int quad = ich/64;    // quadrant
      int pmtch = (ich/16)*8 + ich%8;
      int tq = (ich/8)%2;   // 0 = T-channel, 1 = Q-channel

      if ( pmtch == 127 )
      {
        verbose = 1;
      }

      if ( ievt==4 )
      {
        cout << "ich\t" << ich << "\t" << pmtch << "\t" << verbose << "\t" << time_method[ich] << endl;
      }

      if ( time_method[ich] == 1 ) // Use Template fit to get time
      {
        sig->FitTemplate();
        f_tq[pmtch] = sig->GetTime();
        f_q[pmtch] = sig->GetAmpl();
      }
      else if ( time_method[ich] == 0 ) // Use dCFD method to get time
      {
        //Double_t threshold = 4.0*sig->GetPed0RMS();
        //
        Double_t threshold = 0.5;
        sig->GetSplineAmpl();
        f_tq[pmtch] = sig->dCFD( threshold );
        if ( verbose && ievt < 20 )
        {
          cout << "tq" << pmtch << "(" << ich << ")\t" << f_tq[pmtch] << "\t" << f_q[pmtch] << endl;
        }
        f_tq[pmtch] = f_tq[pmtch] - (TRIG_SAMP-3);
        f_tq[pmtch] *= 17.7623;               // convert from sample to ns (1 sample = 1/56.299 MHz)
        f_q[pmtch] = sig->GetAmpl();
        if ( f_q[pmtch]<24 )
        {
          f_tq[pmtch] = -9999.;
        }

        if ( ievt<10 && ich==255)
        {
          cout << "dcfdcalc " << ievt << "\t" << ich << "\t" << pmtch << "\t" << f_tq[pmtch] << "\t" << f_q[pmtch] << endl;
        }
      }
      else if ( time_method[ich] == 2 ) // Use MBD method to get time
      {
        //Double_t threshold = 4.0*sig->GetPed0RMS();
        //
        Double_t threshold = 0.5;
        Double_t tdc = sig->MBD( TRIG_SAMP );
        f_q[pmtch] = sig->GetSplineAmpl();

        // Convert TDC to ns
//chiu Skip for now
        //f_t0[ich] = tdc2time[ich]->Eval( tdc );
 
        f_tt[pmtch] = 12.5 - tdc*0.00189;  // simple linear correction

        /*
        if ( ievt<10 && ich==0)
        {
          cout << "mbdtcalc " << ievt << "\t" << ich << "\t" << f_tt[pmtch]
            << "\t" << f_tq[pmtch] << "\t" << f_q[pmtch] << endl;
        }
        */

      }

      /*
      if ( verbose )
      {
        ac->cd(ich+1);
        //sig->GetGraph()->GetXaxis()->SetRangeUser(f_t0[ich]-5,f_t0[ich]+5);
        sig->GetGraph()->Draw("alp");

        line[ich].SetX1(f_t0[ich]);
        line[ich].SetY1(0);
        line[ich].SetX2(f_t0[ich]);
        line[ich].SetY1(1);
        line[ich].Draw("same");
      }
      */

      if ( pmtch==127 )
      {
        verbose = 0;
      }
    }

    // calculate bbc global variables
    for (int ipmt=0; ipmt<NPMT; ipmt++)
    {
      int arm = ipmt/64;

      if ( f_q[ipmt] > 20 )
      {
        f_bq[arm] += f_q[ipmt];
        f_bn[arm] += 1;
      }

    }


    if ( verbose )
    {
      gPad->Modified();
      gPad->Update();
    }

    //if ( verbose )
    if ( ievt == 4 )
    {
      cout << "Event " << ievt << "\n0:\t";
      for (int ipmt=0; ipmt<NPMT; ipmt++)
      {
        cout << f_tt[ipmt] << "\t";
        if ( ipmt%8 == 7 )
        {
          cout << endl << ipmt << ":\t";
        }
      }
      cout << endl;

    }   // End of event

    tree->Fill();

    if ( verbose )
    {
      string junk;
      cin >> junk;
      if ( junk[0] == 'q' ) break;
      if ( junk[0]=='s' || junk[0]=='w' )
      {
        name = "calctimes_evt"; name += ievt; name += ".png";
        ac->SaveAs(name);
      }
    }
  }

  savefile->Write();

  return 1;
}

