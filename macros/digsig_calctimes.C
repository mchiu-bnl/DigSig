#include <iostream>

/*
//NEW BBC Electronics
const int NCH = 2;
const int NSAMP = 31;
//TString template_fname = "sig_gen2";
TString template_fname = "TIMING16_TEMPLATE/sig_gen2";
*/

//CAENv1742
const int NCH = 2;
const int NSAMP = 1024;

//DRS4
/*
const int NCH = 4;
const int NSAMP = 1024;
*/
//TString template_fname = "TEMPLATES/sig_gen0";
TString template_fname = "TEMPLATES/sig_gen2";

const int verbose = 0;
TH1 *htdiff[100];

Float_t f_t0[NCH];
Float_t f_adc[NCH];

const int time_method = 0; // 0 = dCFD, 1=template fit

int digsig_calctimes(const char *rootfname = "drs4.root")
{
  gSystem->Load("libdigsig.so");
  //Float_t f_evt;

  TString savefname = rootfname;
  savefname.ReplaceAll(".root","_times.root");
  TFile *savefile = new TFile(savefname,"RECREATE");

  TTree *tree = new TTree("t","DRS4 times");
  //tree->Branch("evt", &f_evt, "evt/I");
  TString name, leaf;
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
  ac->Divide(2,2);
  DigAna digana(NCH,NSAMP);

  int nentries = digana.OpenRootFile(rootfname);

  // Set Calibrations
  TString pedfname = rootfname;
  pedfname.ReplaceAll(".root","_ped.txt");
  Int_t index = pedfname.Last('/');
  if ( index>=0 ) pedfname.Remove(0,index+1);
  pedfname.Prepend("PED/");
  int pedstatus = digana.SetPed0FromFile( pedfname );
  if ( pedstatus<0 )
  {
    cerr << "ERROR: can't open file " << pedfname << endl;
    return 0;
  }

  // Now read in template waveforms
  if ( time_method==1 )
  {
    digana.ReadTemplate(template_fname);
  }

  string junk;
  Double_t ampl, time;
  TLine line[4];

  for (int ievt=0; ievt<nentries; ievt++)
  {
    if ( ievt%1000 == 0 )
    {
      cout << "Event " << ievt << endl;
    }

    digana.ProcessEvent(ievt);

    for (int ich=0; ich<NCH; ich++)
    {
      DigSig *sig = digana.GetSig(ich);

      if ( time_method == 1 ) // Use Template fit to get time
      {
        sig->FitTemplate();
        f_t0[ich] = sig->GetTime();
        f_adc[ich] = sig->GetAmpl();
      }
      else if ( time_method == 0 ) // Use dCFD method to get time
      {
        //Double_t threshold = 4.0*sig->GetPed0RMS();
        Double_t threshold = .5;
        f_t0[ich] = sig->dCFD( threshold );
      }

      if ( verbose )
      {
        ac->cd(ich+1);
        sig->GetGraph()->GetXaxis()->SetRangeUser(f_t0[ich]-5,f_t0[ich]+5);
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
    }
  }

  savefile->Write();
  return 1;
}

