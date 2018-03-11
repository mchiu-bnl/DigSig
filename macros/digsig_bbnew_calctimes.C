#include <iostream>

const int NCH = 2;
const int NSAMP = 31;

void digsig_ana()
{
  gSystem->Load("libdigsig.so");
  DigAna digana(NCH,NSAMP);

  // Set Calibrations
  const Double_t ped0[] = { 1584.3, 1497.5 };
  for (int ich=0; ich<NCH; ich++)
  {
    DigSig *sig = digana.GetSig(ich);
    sig->SetPed0( ped0[ich] );
  }

  TString name = "sig_gen0";
  digana.ReadTemplate(name);

  DigSig *sig0 = digana.GetSig(0);
  TF1 *f1_template = sig0->GetTemplateFcn();
  f1_template->SetParameters(1,8);
  f1_template->Draw();
}

void xxx_digsig_ana(const char *rootfname = "bb_2017_04_03_10_18.root")
{
  gSystem->Load("libdigsig.so");

  TFile *savefile = new TFile("bbana.root","RECREATE");

  TCanvas *ac = new TCanvas("ac","ac",550,425);
  DigAna digana(NCH,NSAMP);

  // Set Calibrations
  const Double_t ped0[] = { 1584.3, 1497.5 };
  for (int ich=0; ich<NCH; ich++)
  {
    DigSig *sig = digana.GetSig(ich);
    sig->SetPed0( ped0[ich] );
  }


  int nentries = digana.OpenRootFile(rootfname);

  string junk;
  Double_t ampl, time;

  for (int ievt=0; ievt<nentries; ievt++)
  {
    digana.ProcessEvent(ievt);
    //digana.FillPed0(0,2);

    for (int ich=0; ich<NCH; ich++)
    {
      DigSig *sig = digana.GetSig(ich);
      sig->FillSplineTemplate(ampl,time);
      //sig.FitPulse();
      /*
         TGraph *g = sig->GetGraph();
         g->Draw("ap");
         gPad->Modified();
         gPad->Update();
         cout << ievt << " ? ";
         cin >> junk;
         if ( junk[0] == 'q' ) break;
      */
    }
  }

  ofstream template_shapefile("bbc.shape");
  ofstream template_sherrfile("bbc.sherr");
  for (int ich=0; ich<NCH; ich++)
  {
    DigSig *sig = digana.GetSig(ich);
    //cout << ich << "\t" << sig->GetPed0() << endl;
    sig->MakeSplineTemplate(template_shapefile,template_sherrfile);
  }


  savefile->Write();
  template_shapefile.close();
  template_sherrfile.close();
}

