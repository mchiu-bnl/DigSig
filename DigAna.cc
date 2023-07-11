#include "DigAna.h"
#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <iostream>
#include <fstream>

using namespace std;

DigAna::DigAna(const int numch, const int nsamp) :
  nch(numch),
  nsamples(nsamp),
  tfile(0)
{
  TString name;

  //nch = numch;
  //nsamples = nsamp;
  invert = 1;

  for (int ich=0; ich<nch; ich++)
  {
    //cout << "Creating digsig " << ich << endl;
    digsig.push_back( DigSig(ich,nsamples) );

    ch_skip.push_back( 0 ); // by default all ch's are good
  }

  f_evt = -1;
  f_clock = 0;
  f_femclock = 0;

  name = "hdTime";
  hdTime = new TH1F(name,name,1000,-0.1,0.1);

  //cout << (unsigned long)f_x << endl;
}

DigAna::~DigAna()
{
  //if ( tfile!=0 ) tfile->Close();
}

// Root TTree format is the kind we used for DRS4
Stat_t DigAna::OpenRootFile(const char *fname)
{
  // Restore original directory after we are done
  TDirectory *orig_dir = gDirectory;

  cout << "DigAna: Opening " << fname << endl;
  tfile = new TFile(fname,"READ");
  if ( tfile==0 )
  {
    cerr << "ERROR, can't open " << fname << endl;
    return 0;
  }

  ttree = (TTree*)tfile->Get("t");
  ttree->SetBranchAddress("evt",&f_evt);
  ttree->SetBranchAddress("clk",&f_clock);
  ttree->SetBranchAddress("femclk",&f_femclock);
  //ttree->SetBranchAddress("spillevt",&f_spillevt);
  //ttree->SetBranchAddress("dtstamp",&f_dtstamp);
  TString label;
  for (int ich=0; ich<nch; ich++)
  {
    label = "t"; label += ich;
    ttree->SetBranchAddress(label,&f_x[ich]);
    label = "ch"; label += ich;
    ttree->SetBranchAddress(label,&f_y[ich]);
  }

  nentries = ttree->GetEntries();

  orig_dir->cd();

  return nentries;
}

void DigAna::FillPed0(const Int_t sampmin, const Int_t sampmax)
{
  for (int ich=0; ich<nch; ich++)
  {
    digsig[ich].FillPed0(sampmin,sampmax);
  }
}

void DigAna::FillPed0(const Double_t xmin, const Double_t xmax)
{
  for (int ich=0; ich<nch; ich++)
  {
    digsig[ich].FillPed0(xmin,xmax);
  }
}

int DigAna::SetPed0FromFile(const char *pedfname)
{
  ifstream pedfile(pedfname);
  if ( pedfile.fail() ) return -1;

  int nped = 0;
  int ch = 0;
  Double_t pedmean;
  Double_t pedrms;
  cout << "Reading in Pedestals from " << pedfname << endl;
  while ( pedfile >> ch >> pedmean >> pedrms )
  {
    if ( ch<0 )
    {
      cerr << __FILE__ << " " << __LINE__ << " ERROR, invalid ch: "
        << ch << endl;
      return ch;
    }
    else if ( ch_skip[ch]==1 || ch >= nch )
    {
      nped++;
      continue;
    }

    // could also check that pedrms is reasonable
    digsig[ch].SetPed0( pedmean, pedrms );
    cout << ch << "\t" << pedmean << "\t" << pedrms << endl;
    nped++;
  }

  return nped;
}

void DigAna::SetEventPed0Range(const Int_t minsamp, const Int_t maxsamp)
{
  for (int ich=0; ich<nch; ich++)
  {
    if ( ch_skip[ich] == 1 ) continue;
    digsig[ich].SetEventPed0Range(minsamp,maxsamp);
  }
}

void DigAna::SetEventPed0Range(const Double_t xmin, const Double_t xmax)
{
  for (int ich=0; ich<nch; ich++)
  {
    if ( ch_skip[ich] == 1 ) continue;
    digsig[ich].SetEventPed0Range(xmin,xmax);
  }
}

// Need to change this so that it's CalcIntegral Around Peak
void DigAna::CalcIntegralAroundPeak(const Double_t leftlimit, const Double_t rightlimit)
{
  for (int ich=0; ich<nch; ich++)
  {
    if ( ch_skip[ich] == 1 ) continue;
    Double_t xpeak; // x value at peak
    Double_t ypeak; // peak y
    digsig[ich].LocMax(xpeak, ypeak); 
    digsig[ich].Integral(xpeak+leftlimit,xpeak+rightlimit);
    //Double_t integral = digsig[ich].Integral(xpeak+leftlimit,xpeak+rightlimit);
    //cout << "ich integral " << ich << " " << integral << endl;
  }
}

void DigAna::SetTemplateMinMaxGoodADC(const Double_t min, const Double_t max)
{
  for (int ich=0; ich<nch; ich++)
  {
    if ( ch_skip[ich] == 1 ) continue;
    digsig[ich].SetTemplateMinMaxGoodADC(min,max);
  }
}

void DigAna::SetTemplateMinMaxFitRange(const Double_t min, const Double_t max)
{
  for (int ich=0; ich<nch; ich++)
  {
    if ( ch_skip[ich] == 1 ) continue;
    digsig[ich].SetTemplateMinMaxFitRange(min,max);
  }
}

void DigAna::SetTimeOffset(const Double_t o)
{
  for (int ich=0; ich<nch; ich++)
  {
    if ( ch_skip[ich] == 1 ) continue;
    digsig[ich].SetTimeOffset( o );
  }
}

void DigAna::SkipAll()
{
  for (int ich=0; ich<nch; ich++)
  {
    ch_skip[ich] = 1;
  }
}

void DigAna::EnableCh(const int ich)
{
  ch_skip[ich] = 0;
}

void DigAna::SkipCh(const int ich)
{
  ch_skip[ich] = 1;
}

int DigAna::ProcessEvent(const int entry)
{
  ttree->GetEntry(entry);

  /*
  cout << "Event " << f_evt << endl;
  for (int ich=0; ich<nch; ich++)
  {
    cout << ich << ":\t";
    for (int isamp=0; isamp<nsamples; isamp++)
    {
      cout << f_x[ich][isamp] << " ";
    }
    cout << endl;
    cout << ich << ":\t";
    for (int isamp=0; isamp<nsamples; isamp++)
    {
      cout << f_y[ich][isamp] << " ";
    }
    cout << endl;
  }
  */

  for (int ich=0; ich<nch; ich++)
  {
    if ( ch_skip[ich] == 1 ) continue;
    //if ( entry<1 ) cout << "DigSig::SetXY(), ch " << ich << endl;
    digsig[ich].SetXY(f_x[ich],f_y[ich],invert);
  }

  return 1;
}

void  DigAna::SetTemplateSize(const Int_t nptsx, const Int_t nptsy, const Double_t begt, const Double_t endt)
{
  for (int ich=0; ich<nch; ich++)
  {
    if ( ch_skip[ich] == 1 ) continue;
    digsig[ich].SetTemplateSize(nptsx,nptsy,begt,endt);
  }
}

// Be sure to call OpenRootFile first
void DigAna::FillSplineTemplate()
{
  //cout << "In DigAna::FillSplineTemplate " << endl;
  Int_t nentries = ttree->GetEntries();

  for (int ievt=0; ievt<nentries; ievt++)
  {
    ProcessEvent(ievt);

    for (int ich=0; ich<nch; ich++)
    {
      //cout << "ch " << ich << "xxx" << endl;
      if ( ch_skip[ich] == 1 ) continue;
      DigSig *sig = GetSig(ich);
      sig->FillSplineTemplate();
    }
  }
}

// Be sure to call OpenRootFile first
// and to read in a template
void DigAna::FillFcnTemplate()
{
  Int_t nentries = ttree->GetEntries();

  Double_t *time = new Double_t[nch];

  for (int ievt=0; ievt<nentries; ievt++)
  {
    //cout << "event " << ievt << " / " << nentries << endl;
    ProcessEvent(ievt);

    for (int ich=0; ich<nch; ich++)
    {
      if ( ch_skip[ich] == 1 ) continue;
      DigSig *sig = GetSig(ich);
      sig->FillFcnTemplate();

      time[ich] = sig->GetTime();
    }

    hdTime->Fill( time[1] - time[0] );
  }

  delete[] time;
  //cout << "Exiting DigAna::FilLFcnTemplate" << endl;
}

void DigAna::MakeAndWriteTemplate(const char *savename)
{
  TString shname = savename; shname += ".shape";
  TString sherrname = savename; sherrname += ".sherr";
  ofstream template_shapefile( shname );
  ofstream template_sherrfile( sherrname );

  for (int ich=0; ich<nch; ich++)
  {
    if ( ch_skip[ich] == 1 ) continue;
    DigSig *sig = GetSig(ich);
    //cout << ich << "\t" << sig->GetPed0() << endl;
    sig->MakeAndWriteTemplate(template_shapefile,template_sherrfile);
  }

  template_shapefile.close();
  template_sherrfile.close();

}

void DigAna::ReadTemplate(const char *savedname)
{
  string shname = savedname; shname += ".shape";
  string sherrname = savedname; sherrname += ".sherr";

  ifstream shapefile(shname.c_str());
  ifstream sherrfile(sherrname.c_str());

  for (int ich=0; ich<nch; ich++)
  {
    cout << "Reading Template " << ich << endl;
    digsig[ich].ReadTemplate( shapefile, sherrfile );
  }

  shapefile.close();
  sherrfile.close();
}

