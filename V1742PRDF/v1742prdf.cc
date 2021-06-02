
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <pmonitor/pmonitor.h>
#include "caen_calib.h"
#include "v1742prdf.h"

#include <TFile.h>
#include <TTree.h>
#include <TString.h>
//#include <TH1.h>

int init_done = 0;

using namespace std;

const int MAX_PACKETS = 8; // Maximum number of packets (ie, boards)
const int MAX_DRS = 4;     // Maximum number of DRS4 per board
const int MAX_CALCH = 9;   // Maximum number of ch per DRS4
const int NCHPERBOARD = 34; // Number of channels in one V1742 (32 + 2 Trig)

const int NBOARDS = 8;
const int NCH = NBOARDS*NCHPERBOARD;      // number active channels
const int NSAMPLES = 1024; // num samples in a waveform

int chmap[NCH];

//TH1F *trace[64];
TFile *savefile;
TTree *t;
Int_t f_evt;
UInt_t f_tstamp;
Short_t f_spill;
Short_t f_spillevt;
Float_t f_volt[NCH][NSAMPLES];
Float_t f_time[NCH][NSAMPLES];

CAEN_Calib *caen_calib[MAX_PACKETS];


/*
 // laser config
const char *caen_calibfname[MAX_PACKETS] = {
  "caen_calibration/calib_0097_5G.dat",
  "caen_calibration/calib_0106_5G.dat",
  "caen_calibration/calib_0081_5G.dat",
  "caen_calibration/calib_0087_5G.dat"
};
*/

/*
const char *caen_calibfname[MAX_PACKETS] = {
  "caen_calibration/calib_12064_5G.dat",
  "caen_calibration/calib_0106_5G.dat",
  "caen_calibration/calib_10906_5G.dat",
  "caen_calibration/calib_12067_5G.dat",
  "caen_calibration/calib_0081_5G.dat",
  "caen_calibration/calib_0097_5G.dat",
  "caen_calibration/calib_0120_5G.dat",
  "caen_calibration/calib_0087_5G.dat"
};
*/


int SaveFile();

void LoadCorrections()
{
  ifstream infile("caen.list");
  int serial_no = 0;


  char caen_calibfname[1024];
  for (int iboard=0; iboard<MAX_PACKETS; iboard++)
  {
    infile >> serial_no;
    sprintf(caen_calibfname,"caen_calibration/calib_%04d_5G.dat",serial_no);
    //cout << "Loading " << caen_calibfname << endl;
    caen_calib[iboard] = new CAEN_Calib( caen_calibfname );
  }

  infile.close();
}

int SetChannelMapFile(const char *chfname)
{
  if ( strlen(chfname) == 0 )
  {
    // Default is straight ordering
    for (int ich=0; ich<NCH; ich++)
    {
      chmap[ich] = ich;
    }
  }
  else
  {
    // get chmap fromfile
    ifstream chmapfile(chfname);
    int tempch;
    int n = 0;
    while ( chmapfile >> tempch )
    {
      cout << n << "\t" << tempch << endl;
      chmap[n++] = tempch;
    }
  }

  return 1;
}

int SaveFile()
{
  cout << "Saving File, nevents = " << t->GetEntries() << endl;
  if ( savefile )
  {
    savefile->Write();
    savefile->Close();
  }

  return 0;
}

int pinit()
{
  cout << "v1742prdf pinit()" << endl;

  if (init_done) return 1;
  init_done = 1;

  TString name;
  TString leaflist;

  cout << "Creating v1742.root" << endl;
  savefile = new TFile("v1742.root","RECREATE");

  t = new TTree("t","DT5742 data");
  t->Branch("evt",&f_evt,"evt/I");
  t->Branch("tstamp",&f_tstamp,"tstamp/i");
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

  LoadCorrections();

  return 0;
}

int process_event(Event * e)
{
  int evt_type = e->getEvtType();
  
  // Start run
  if ( evt_type == 9 )
  {
    cout << "Found Begin Run Event" << endl;
    return 0;
  }
  else if ( evt_type == 12 )
  {
    cout << "Found End Run Event" << endl;
    return 0;
  }

  f_evt = e->getEvtSequence();
  if ( (f_evt%100) == 0 )
  {
    cout << "Processing event " << f_evt << endl;
  }


  if ( evt_type != 1 ) return 0;


  Packet *p[MAX_PACKETS] = {0};

  // Get packets
  for (int iboard=0; iboard<MAX_PACKETS; iboard++)
  {
    //p[iboard] = e->getPacket(2001+iboard);
    p[iboard] = e->getPacket(2001+iboard);
    //cout << "Found packet " << 2001+iboard << "\t" << p[iboard] << endl;
 
    if ( p[iboard] )
    {
      caen_calib[iboard]->apply_calibs( p[iboard] );

      for (int ich=0; ich<NCHPERBOARD; ich++)
      {
        int feech = iboard*NCHPERBOARD + ich;
        //cout << feech << endl;
        for (int isamp=0; isamp<NSAMPLES; isamp++)
        {
          f_volt[feech][isamp] = caen_calib[iboard]->corrected(ich, isamp);
          f_time[feech][isamp] = caen_calib[iboard]->caen_time(ich, isamp);
        }
      }
    }

  }

  /*
  if ( p[0] )
  {
    f_evt = p[0]->iValue(0,"EVNR");
    if ( f_evt%1000 == 0 )
    {
      cout << "Event " << f_evt << endl;
    }

    //// Dump for debugging
    //p[0]->dump();
    //string junk;
    //cout << "? ";
    //cin >> junk;
  }
  else
  {
    cout << "Packet 2000 NOT FOUND " << endl;
    return 0;
  }
  */

  /*
  // For debugging
  int s1 = p[0]->iValue(100, 0);
  int s2 = p[0]->iValue(100, 8);
  cout << f_evt << "\t" << s1 << "\t" << s2 << endl;
  */

  t->Fill();

  // Delete the packets
  for (int ipkt=0; ipkt<MAX_PACKETS; ipkt++)
  {
    if ( p[ipkt]!=0 ) delete p[ipkt];
  }

  return 0;

}

