#include <iostream>

/*
//NEW BBC Electronics
const int NCH = 2;
const int NSAMP = 31;
//TString template_fname = "sig_gen2";
TString template_fname = "TIMING16_TEMPLATE/sig_gen2";
*/

//DRS4
//const int NCH = 4;
//const int NSAMP = 1024;

//MBD DS TEST
const int NCH = 64;
const int NSAMP = 20;

/*
//DT5742
const int NCH = 16;
const int NSAMP = 1024;
*/

TString template_fname = "TEMPLATES/sig_gen0";
//TString template_fname = "TEMPLATES/sig_gen2";

const int verbose = 0;
//TH1 *htdiff[NCH];

Int_t   f_evt;
Float_t f_t0[NCH];
Float_t f_adc[NCH];

int time_method[NCH];
const int MAX_SAMP = 6;     // use this sample to get the MBD time

// 0 = dCFD, 1=template fit, 2=MBD-method
void set_time_method()
{
  for (int ich=0; ich<8; ich++)
  {
    time_method[ich] = 2;
  }
  for (int ich=8; ich<16; ich++)
  {
    time_method[ich] = 0;
  }
}

TF1 *tdc2time[NCH];   // converts tdc to time
void read_tcalib(const char *tcalibfname = "mbd.tcalib")
{
  cout << "Reading TDC 2 Time(ns) calibrations from " << tcalibfname << endl;
  ifstream tcalibfile( tcalibfname );

  for (int ich=0; ich<NCH; ich++)
  {
    //tdc2time[ich] = new TF1("name","pol2",0,30);
    tdc2time[ich] = new TF1("name","pol3",0,30);

    Int_t ch;
    Double_t p0, p1, p2, p3;
    Double_t p0err, p1err, p2err, p3err;
    tcalibfile >> ch >> p0 >> p0err >> p1 >> p1err >> p2 >> p2err >> p3 >> p3err;
    cout << ch << "\t" << p0 << "\t" << p0err << "\t" << p1 << "\t" << p1err << "\t" << p2 << "\t" << p2err
      << "\t" << p3 << "\t" << p3err << endl;

    tdc2time[ch]->SetParameters(p0,p1,p2,p3);
  }
}

int digsig_calctimes(const char *rootfname = "drs4.root", const int nevents = 0)
{
  gSystem->Load("libdigsig.so");
  //Float_t f_evt;

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
  for (int ich=0; ich<16; ich++)
  {
    digana.EnableCh(4);
    digana.EnableCh(6);
    digana.EnableCh(12);
    digana.EnableCh(14);
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
  TLine line[NCH];

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
        f_t0[ich] = sig->MBD( MAX_SAMP );
        f_adc[ich] = sig->GetAmpl();
        //cout << ievt << "\t" << ich << "\t" << f_t0[ich] << "\t" << f_adc[ich] << endl;
        if ( f_adc[ich]<20 ) f_t0[ich] = -9999.;
        else
        {
          // Convert TDC to ns
          f_t0[ich] = tdc2time[ich]->Eval( f_t0[ich] );
          //f_t0[ich] = f_t0[ich]*0.00189;  // simple linear correction
        }
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

  savefile->Write();
  return 1;
}

