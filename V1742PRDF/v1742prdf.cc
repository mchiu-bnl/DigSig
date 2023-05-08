
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
#include <TH1.h>

int init_done = 0;

using namespace std;

const int MAX_PACKETS = 10; // Maximum number of packets (ie, boards)
const int MAX_DRS = 4;     // Maximum number of DRS4 per board
const int MAX_CALCH = 9;   // Maximum number of ch per DRS4
const int NCHPERBOARD = 34; // Number of channels in one V1742 (32 + 2 Trig)

const int NBOARDS = 8;
//const int NBOARDS = 10;
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


int SaveFile();

// Check alignment by looking for mis-alignment in TR0 and TR1 channels
TH1 *h_tdiff = 0;
Float_t ref_times[NBOARDS][2];
void CheckAlignment()
{

  static int first_event = 1;
  if ( first_event==1 )
  {
    cout << "CHECKING BOARD ALIGNMENT" << endl;

    h_tdiff = new TH1F("h_tdiff","TR tdiffs", 200, -100,100);
  }

  h_tdiff->Reset();

  // Board Alignment Check
  float tr0_time = 0;
  for (int iboard=0; iboard<NBOARDS; iboard++)
  {

    for (int ich=32; ich<34; ich++)
    {
      int feech = iboard*34 + ich;

      // randomly pick samples 10-40 (2-8ns)for pedestal
      float ped = 0.;
      float npedsamp = 0;
      for (int isamp=10; isamp<40; isamp++)
      {
        ped += f_volt[feech][isamp];
        npedsamp += 1.0;
      }
      ped /= npedsamp;

      int trig = 0;
      float threshold = 100.;
      for (int isamp=0; isamp<NSAMPLES; isamp++)
      {
        
        if ( fabs( f_volt[feech][isamp]-ped ) > threshold && trig==0 )
        {
          trig = isamp;
          if (iboard==0 && ich==32)
          {
            tr0_time = trig;
          }
          if ( first_event == 1 )
          {
            // Get initial deltas
            ref_times[iboard][ich-32] = trig - tr0_time;
            cout << "Reftime " << iboard << "\t" << feech << "\t" << ref_times[iboard][ich-32] << endl;
          }

          break;
        }

      }

      // Fill tdiffs
      if ( trig>0 )
      {
        cout << feech << "\t" << trig << "\t" << tr0_time << "\t" << ref_times[iboard][ich-32]
          << trig - tr0_time - ref_times[iboard][ich-32] << endl;
        h_tdiff->Fill( trig - tr0_time - ref_times[iboard][ich-32] );
      }
      else
      {
        cout << "ERROR, no trigger " << f_evt << "\t" << iboard << " " << feech << endl;
      }

    }
  }

  // Check for high RMS
  Double_t tdiff_rms = h_tdiff->GetRMS();
  if ( tdiff_rms != 0 )
  {
    cout << "RMS " << f_evt << "\t" << tdiff_rms << endl;
    if ( tdiff_rms > 3.0 )
    {
      h_tdiff->Print("ALL");
    }
  }


  if ( first_event==1 )
  {
    first_event = 0;
  }
}

void LoadCorrections()
{
  ifstream infile("caen.list");
  int serial_no = 0;
  int freq = 0;


  char caen_calibfname[1024];
  for (int iboard=0; iboard<MAX_PACKETS; iboard++)
  {
    infile >> serial_no >> freq;
    sprintf(caen_calibfname,"caen_calibration/calib_%04d_%1dG.dat",serial_no,freq);
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

  t->Fill();

  // Delete the packets
  for (int ipkt=0; ipkt<MAX_PACKETS; ipkt++)
  {
    if ( p[ipkt]!=0 ) delete p[ipkt];
  }

  //CheckAlignment();

  return 0;

}

