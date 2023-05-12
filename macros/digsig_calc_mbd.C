#include <iostream>

/*
//NEW BBC Electronics
const int NCH = 2;
const int NSAMP = 31;
//TString template_fname = "sig_gen2";
TString template_fname = "TIMING16_TEMPLATE/sig_gen2";
*/

//MBD DS TEST
const int MAXCH = 256;
const int NSAMP = 20;
int NCH;    // number of ch's to process

TString template_fname = "TEMPLATES/sig_gen0";
//TString template_fname = "TEMPLATES/sig_gen2";

const int verbose = 0;
//TH1 *htdiff[NCH];

Int_t   f_evt;
Float_t f_t0[MAXCH];
Float_t f_adc[MAXCH];

int time_method[MAXCH];
const int MAX_SAMP = 6;     // use this sample to get the MBD time

// 0 = dCFD, 1=template fit, 2=MBD-method
void set_time_method()
{
  for (int ich=0; ich<NCH; ich++)
  {
    //int chtype = (ich/8)%2;   // 0=time ch, 1=charge ch
    int chtype = (ich/16)%2;   // 0=time ch, 1=charge ch

    if ( chtype==0 ) time_method[ich] = 2;  // time ch
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

int digsig_calc_mbd(const char *rootfname = "drs4.root", const int nevents = 0)
{
  ofstream csv_timefile("case17_delay109_20210915_laser_time.csv");
  ofstream csv_chfile("case17_delay109_20210915_laser_charge.csv");

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

  read_tcalib();

  TString savefname = rootfname;
  savefname.ReplaceAll(".root","_times.root");
  TFile *savefile = new TFile(savefname,"RECREATE");

  TString name, leaf;
  TTree *tree = new TTree("t","DigSig times");
  tree->Branch("evt", &f_evt, "evt/I");
  for (int ich=0; ich<NCH; ich++)
  {
    name = "t"; name += ich; 
    leaf = name; leaf += "/F";
    tree->Branch(name, &f_t0[ich], leaf);
    name = "ch"; name += ich; 
    leaf = name; leaf += "/F";
    tree->Branch(name, &f_adc[ich], leaf);
  }

  TCanvas *ac = new TCanvas("ac","ac",800,800);
  ac->Divide(4,4);

  DigAna digana(NCH,NSAMP);

  // Enable only certain channels for analysis
  /*
  digana.SkipAll();
  for (int ich=0; ich<8; ich++)
  {
    digana.EnableCh(4);
    digana.EnableCh(6);
    digana.EnableCh(12);
    digana.EnableCh(14);
  }
  */
  /*
  for (int ich=0; ich<8; ich++)
  {
    digana.EnableCh(ich);
  }
  for (int ich=16; ich<24; ich++)
  {
    digana.EnableCh(ich);
  }
  */

  // Set Time Reco Method to use
  set_time_method();

  int nentries = digana.OpenRootFile( rootfname );
  if ( nevents != 0 ) nentries = nevents;

  // Set Calibrations
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

    for (int ich=0; ich<NCH; ich++)
    {
      DigSig *sig = digana.GetSig(ich);

      if ( time_method[ich] == 1 ) // Use Template fit to get time
      {
        sig->FitTemplate();
        f_t0[ich] = sig->GetTime();
        f_adc[ich] = sig->GetAmpl();
      }
      else if ( time_method[ich] == 0 ) // Use dCFD method to get time
      {
        //Double_t threshold = 4.0*sig->GetPed0RMS();
        //
        Double_t threshold = 0.5;
        sig->GetSplineAmpl();
        f_t0[ich] = sig->dCFD( threshold );
        f_adc[ich] = sig->GetAmpl();
        if ( f_adc[ich]<20 ) f_t0[ich] = -9999.;
      }
      else if ( time_method[ich] == 2 ) // Use MBD method to get time
      {
        //Double_t threshold = 4.0*sig->GetPed0RMS();
        //
        Double_t threshold = 0.5;
        //const int MAX_SAMP = 8;     // use this sample to get the time
        Double_t tdc = sig->MBD( MAX_SAMP );
        f_adc[ich] = sig->GetAmpl();
        //cout << ievt << "\t" << ich << "\t" << f_t0[ich] << "\t" << f_adc[ich] << endl;
        if ( f_adc[ich]<20 ) f_t0[ich] = -9999.;
        else
        {
          // Convert TDC to ns
          f_t0[ich] = tdc2time[ich]->Eval( tdc );
          //f_t0[ich] = f_t0[ich]*0.00189;  // simple linear correction
        }

      }

      // Here we dump the output to csv file
      if ( ich>=16 && ich<24 && f_t0[ich-16]>-100 && f_t0[ich-16]<31 )
      {
        csv_timefile << f_evt << "," << ich-16 << ",";
        csv_chfile << f_evt << "," << ich-16 << ",";

        // Save the charge pulse
        DigSig *sig = digana.GetSig(ich-16);
        TGraphErrors *g = sig->GetGraph();
        Double_t *yval = g->GetY();
        for (int ix=0; ix<g->GetN(); ix++)
        {
          csv_timefile << yval[ix] << ",";
        }

        // Save the charge pulse
        sig = digana.GetSig(ich);
        g = sig->GetGraph();
        yval = g->GetY();
        for (int ix=0; ix<g->GetN(); ix++)
        {
          csv_chfile << yval[ix] << ",";
        }

        csv_timefile<< f_t0[ich-16] << endl;
        csv_chfile<< f_adc[ich] << endl;
      }

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
    }

    if ( verbose )
    {
      gPad->Modified();
      gPad->Update();
    }

    if ( verbose )
    {
      cout << "Event " << ievt << "\t";
      for (int ich=0; ich<NCH; ich++)
      {
        cout << f_t0[ich] << "\t";
      }
      cout << endl;
    }

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

  if ( csv_chfile.good() )
  {
    csv_timefile.close();
    csv_chfile.close();
  }

  savefile->Write();

  return 1;
}

