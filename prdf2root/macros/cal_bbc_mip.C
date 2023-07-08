#include "get_runstr.h"

void cal_bbc_mip(const char *tfname = "beam_mbd-00009184-0000_mbd.root", const int pass = 0, const int nevt = 0)
{
  cout << "tfname " << tfname << endl;

  const int NUM_PMT = 128;
  //const int NUM_PMT = 12;
  const int NUM_ARMS = 2;

  // Set up variables to read from TTree
  Int_t   f_evt;
  Float_t f_tt[NUM_PMT];  // time from t-channels
  Float_t f_tq[NUM_PMT];  // time from q-channels
  Float_t f_q[NUM_PMT];   // charge

  Short_t f_bn[NUM_ARMS];   // num hit PMTs
  Float_t f_bq[NUM_ARMS];   // chargesum
  Float_t f_bt[NUM_ARMS];   // mean time in arm
  Float_t f_bz;             // z-vertex
  Float_t f_bt0;            // t-zero

  // Set up TTree
  TString name;
  TFile *tfile = new TFile(tfname,"READ");
  TTree *tree = (TTree*)tfile->Get("t");
  tree->SetBranchAddress("evt",&f_evt);
  for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
  {
    name = "tt"; name += ipmt;
    tree->SetBranchAddress(name,&f_tt[ipmt]);
    name = "tq"; name += ipmt;
    tree->SetBranchAddress(name,&f_tq[ipmt]);
    name = "q"; name += ipmt;
    tree->SetBranchAddress(name,&f_q[ipmt]);
  }
  tree->SetBranchAddress("bns",&f_bn[0]);
  tree->SetBranchAddress("bnn",&f_bn[1]);
  tree->SetBranchAddress("bqs",&f_bq[0]);
  tree->SetBranchAddress("bqn",&f_bq[1]);
  tree->SetBranchAddress("bts",&f_bt[0]);
  tree->SetBranchAddress("btn",&f_bt[1]);
  tree->SetBranchAddress("bz",&f_bz);
  tree->SetBranchAddress("bt0",&f_bt0);

  //== Create output objects
  TString dir = "results/";
  dir += get_runstr(tfname);
  dir += "/";
  name = "mkdir -p " + dir;
  gSystem->Exec( name );

  name = dir + "calbbc_mip.root";

  TFile *savefile = new TFile(name,"RECREATE");

  TH1 *h_q[NUM_PMT];
  TH1 *h_tq[NUM_PMT];

  TString title;
  for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
  {
    name = "h_q"; name += ipmt;
    title = "q"; title += ipmt;
    h_q[ipmt] = new TH1F(name,title,15100/4,-100,15000);

    name = "h_tq"; name += ipmt;
    title = "tq"; title += ipmt;
    h_tq[ipmt] = new TH1F(name,title,900,-150,150);
  }
  TH2 *h2_tq = new TH2F("h2_tq","ch vs tq",900,-150,150,NUM_PMT,-0.5,NUM_PMT-0.5);

  // Load in calib constants
  Float_t t0_offsets[NUM_PMT] = {};

  if ( pass==1 )
  {
    name = dir + "bbc_t0.calib";
    ifstream calin_t0_file( name );
    int pmtnum;
    float meanerr;
    float sigma;
    float sigmaerr;
    for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
    {
      calin_t0_file >> pmtnum >> t0_offsets[ipmt] >> meanerr >> sigma >> sigmaerr;
      if ( pmtnum != ipmt )
      {
        cerr << "ERROR, pmtnum != ipmt, " << pmtnum << "\t" << ipmt << endl;
        return;
      }
    }
  }
 
  // Event loop, each ientry is one triggered event
  int nentries = tree->GetEntries();
  for (int ientry=0; ientry<nentries; ientry++)
  {
    tree->GetEntry(ientry);

    /*
    if (ientry<4)
    {
      // print charge from channels 0 and 127
      cout << f_evt << "\t" << f_q[0] << "\t" << f_q[127] << endl;
    }
    */

    for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
    {
      float tq = f_tq[ipmt] - t0_offsets[ipmt];

      h_tq[ipmt]->Fill( tq );
      h2_tq->Fill( tq, ipmt );

      if ( pass==0 && tq>-15 && tq<60 )
      {
        h_q[ipmt]->Fill( f_q[ipmt] );
      }
      else if ( pass==1 && fabs(tq)<15 )
      {
        h_q[ipmt]->Fill( f_q[ipmt] );
      }
    }
  }

  TCanvas *ac[100];
  int cvindex = 0;

  //tmax
  ac[cvindex] = new TCanvas("cal_tq","ch vs tq",425*1.5,550*1.5);
  h2_tq->Draw("colz");
  if ( pass==0 )
  {
    name = dir + "h2_tq.png";
  }
  else
  {
    name = dir + "h2_tqcorr.png";
    h2_tq->GetXaxis()->SetRangeUser(-20,20);
    gPad->Modified();
    gPad->Update();
  }
  cout << name << endl;
  ac[cvindex]->Print( name );
  ++cvindex;

  /*
  ac[cvindex] = new TCanvas("cal_q","q",550*1.5,425*1.5);
  gPad->SetLogy(1);

  ofstream cal_t0_file;
  if ( pass==0 ) 
  {
    name = dir + "bbc_t0.calib";
    cal_t0_file.open( name );
  }
  TF1 *gaussian = new TF1("gaussian","gaus",-150,150);
  gaussian->SetLineColor(2);
  for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
  {
    Double_t peak = h_tq[ipmt]->GetMaximum();
    int peakbin = h_tq[ipmt]->GetMaximumBin();
    Double_t mean = h_tq[ipmt]->GetBinCenter( peakbin );
    Double_t sigma = 1.0;
    gaussian->SetParameters( peak, mean, 5 );
    gaussian->SetRange( mean-3*sigma, mean+3*sigma );

    h_tq[ipmt]->Fit(gaussian,"R");
    //gPad->SetLogy(1);
    mean = gaussian->GetParameter(1);
    Double_t meanerr = gaussian->GetParError(1);
    sigma = gaussian->GetParameter(2);
    Double_t sigmaerr = gaussian->GetParError(2);

    if ( pass==0 )
    {
      cal_t0_file << ipmt << "\t" << mean << "\t" << meanerr << "\t" << sigma << "\t" << sigmaerr << endl;
      name = dir + "h_tq"; name += ipmt; name += ".png";
    }
    else
    {
      name = dir + "h_tqcorr"; name += ipmt; name += ".png";
    }
    cout << name << endl;
    ac[cvindex]->Print( name );
  }
  if ( pass==0 )
  {
    cal_t0_file.close();
  }
  ++cvindex;
  */

  //q
  ac[cvindex] = new TCanvas("cal_q","q",425*1.5,550*1.5);
  if ( pass==0 )
  {
    gPad->SetLogy(1);
  }
  else if ( pass==1 )
  {
    ac[cvindex]->Divide(1,2);
    ac[cvindex]->cd(1);
  }

  ofstream cal_t0_file;
  if ( pass==0 ) 
  {
    name = dir + "bbc_t0.calib";
    cal_t0_file.open( name );
  }

  ofstream cal_mip_file;
  if ( pass==1 ) 
  {
    name = dir + "bbc_mip.calib";
    cal_mip_file.open( name );
  }

  //TF1 *mipfit = new TF1("mipfit","gaus+expo(3)",100,600);
  //TF1 *mipfit = new TF1("mipfit","gaus+pol4(3)",100,600);
  //TF1 *mipfit = new TF1("mipfit","gaus+pol4(3)+expo(8)",25,600);
  //TF1 *mipfit = new TF1("mipfit","gaus+pol2(3)+expo(6)",25,600);
  //TF1 *mipfit = new TF1("mipfit","gaus+pol3(3)+expo(7)",25,600);
  TF1 *mipfit = new TF1("mipfit","landau",25,1400);
  mipfit->SetLineColor(4);

  /*
  TF1 *f_expo = new TF1("f_expo","expo",25,50);
  f_expo->SetParameters(1,-1);
  f_expo->SetLineColor( 2 );

  TF1 *f_bkg = new TF1("f_bkg","pol4+expo(5)",25,600);
  f_expo->SetLineColor( 5 );
  */

  TH1 *h_bkg[NUM_PMT];  // background histogram
  TH1 *h_mip[NUM_PMT];  // mip signal histogram
  TH1 *h_bkgmip[NUM_PMT];  // bkg + fit histogram

  //TSpectrum *s = new TSpectrum(1);
  for (int ipmt=0; ipmt<NUM_PMT; ipmt++)
  {

    if ( pass==0 )
    {
      h_q[ipmt]->Draw();

      name = dir + "h_q"; name += ipmt; name += ".png";
      cout << name << endl;
      ac[cvindex]->Print( name );
    }
    else if (pass==1)
    {
      h_bkg[ipmt] = (TH1*)h_q[ipmt]->Clone();
      h_bkg[ipmt]->SetLineColor(2);
      h_q[ipmt]->GetXaxis()->SetRangeUser( 25, 1400 );
      h_q[ipmt]->Sumw2();

      double sigma = 8;
      TSpectrum s{};
      h_bkg[ipmt] = s.Background( h_q[ipmt] );
      
      h_mip[ipmt] = (TH1*)h_q[ipmt]->Clone();
      h_mip[ipmt]->Add( h_bkg[ipmt], -1.0 );

      /*
      Int_t nfound = s.Search(h_q[ipmt],sigma,"",0.1);
      Double_t *xpeaks = s.GetPositionX();

      //h_bkg[ipmt] = s.Background( h_q[ipmt] );

      double best_peak = xpeaks[0];
      if ( best_peak < 50. )
      {
        best_peak = xpeaks[1];
      }

      cout << "peaks\t" << ipmt << "\t" << nfound << "\t" << best_peak << endl;
      */


      // Fit the exponential at low q
      mipfit->SetParameter(1,100);
      h_mip[ipmt]->Fit( mipfit, "R" );

      double integ = mipfit->GetParameter(0);
      double best_peak = mipfit->GetParameter(1);
      double width = mipfit->GetParameter(2);
      double integerr = mipfit->GetParError(0);
      double best_peakerr = mipfit->GetParError(1);
      double widtherr = mipfit->GetParError(2);
      double chi2 = mipfit->GetChisquare();
      double ndf = mipfit->GetNDF();

      cal_mip_file << ipmt << "\t" << integ << "\t" << best_peak << "\t" << width << "\t"
        << integerr << "\t" << best_peakerr << "\t" << widtherr << "\t"
        << chi2/ndf << endl;

      // Get full fit
      h_bkgmip[ipmt] = (TH1*)h_bkg[ipmt]->Clone();
      h_bkgmip[ipmt]->Add( mipfit );
      h_bkgmip[ipmt]->SetLineColor( 8 );

      /*
      gPad->Modified();
      gPad->Update();
      //mipfit->SetRange( xpeaks[0]-2.5*sigma, 600 );
      mipfit->SetRange( 25, 600 );
      mipfit->SetParameter( 0, 1000 );
      mipfit->SetParameter( 1, best_peak );
      mipfit->SetParameter( 2, sigma );
 
      mipfit->SetParameter( 8, f_expo->GetParameter(0) );
      mipfit->SetParameter( 9, f_expo->GetParameter(1) );
      h_q[ipmt]->Fit( mipfit, "R" );
      */


      /*
      int npar = f_bkg->GetNpar();
      for (int ipar=0; ipar<npar; ipar++)
      {
        f_bkg->SetParameter( ipar, mipfit->GetParameter(ipar+3) );
      }
      */

      // Now draw the full dist, plus fit
      ac[cvindex]->cd(1);
      gPad->SetLogy(1);
      h_q[ipmt]->GetXaxis()->SetRangeUser( 0, 1400 );
      //h_q[ipmt]->SetMinimum(10.);
      h_q[ipmt]->Draw();
      h_bkg[ipmt]->Draw("same");
      h_bkgmip[ipmt]->Draw("same");

      gPad->Modified();
      gPad->Update();

      ac[cvindex]->cd(2);
      h_mip[ipmt]->Draw();
      gPad->Modified();
      gPad->Update();



      /*
      string junk;
      cout << "? ";
      cin >> junk;
      */

      name = dir + "h_qfit"; name += ipmt; name += ".png";
      cout << name << endl;
      ac[cvindex]->Print( name );
    }

  }
  ++cvindex;

  if ( pass==0 )
  {
    cal_t0_file.close();
  }
  else if ( pass==1 )
  {
    cal_mip_file.close();
  }

  savefile->Write();
  savefile->Close();
}

