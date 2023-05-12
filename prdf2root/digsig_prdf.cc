
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <pmonitor/pmonitor.h>
#include "digsig_prdf.h"

#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TH1.h>

int init_done = 0;

using namespace std;

const int MAX_PACKETS = 2;    // Maximum number of packets (ie, boards)
const int NCHPERPKT = 128;     // Number of channels in one packet
const int NPACKETS = 2;
const int NCH = NPACKETS*NCHPERPKT;      // number active channels
const int NSAMPLES = 31;      // num samples in a waveform

int chmap[NCH];

//TH1F *trace[64];
TFile *savefile;
TTree *t;
Int_t f_run;
Int_t f_evt;
Int_t f_ch;
//UInt_t f_tstamp;
//Short_t f_spill;
//Short_t f_spillevt;
Float_t f_volt[NCH][NSAMPLES];
Float_t f_time[NCH][NSAMPLES];

//int SaveFile();

// Reset to start fresh at the beginning of the event
void ResetEvent()
{
  f_evt = 0;
  memset(f_volt,0,sizeof(f_volt));
  memset(f_time,0,sizeof(f_time));
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
  cout << "prdf pinit()" << endl;

  if (init_done) return 1;
  init_done = 1;

  TString name;
  TString leaflist;

  cout << "Creating prdf.root" << endl;
  savefile = new TFile("prdf.root","RECREATE");

  t = new TTree("t","PRDF Data");
  t->Branch("run",&f_run,"run/I");
  t->Branch("evt",&f_evt,"evt/I");
  //t->Branch("tstamp",&f_tstamp,"tstamp/i");
  //t->Branch("spill",&f_spill,"spill/S");
  //t->Branch("spillevt",&f_spillevt,"spillevt/S");

  for (int ich=0; ich<NCH; ich++)
  {
    name = "ch"; name += ich;
    leaflist = name; leaflist += "["; leaflist += NSAMPLES; leaflist += "]/F";
    t->Branch(name,f_volt[ich],leaflist);    // waveform value in volts

    name = "t"; name += ich;
    leaflist = name; leaflist += "["; leaflist += NSAMPLES; leaflist += "]/F";
    t->Branch(name,f_time[ich],leaflist);    // waveform times
  }

  //LoadCorrections();

  return 0;
}

int process_event(Event * e)
{
  int evt_type = e->getEvtType();

  // Start run
  if ( evt_type == 9 )
  {
    f_run = e->getRunNumber();
    cout << "Found Begin Run Event for Run " << f_run << endl;
    return 0;
  }
  else if ( evt_type == 12 )
  {
    cout << "Found End Run Event" << endl;
    return 0;
  }

  ResetEvent();

  f_evt = e->getEvtSequence();
  if ( (f_evt%100) == 0 )
  {
    cout << "Processing event " << f_evt << endl;
  }

  if ( evt_type != 1 ) return 0;

  Packet *p[MAX_PACKETS] = {0};

  // Get packets
  int flag_err = 0;
  for (int ipkt=0; ipkt<MAX_PACKETS; ipkt++)
  {
    int pktid = 1001 + ipkt;    // packet id
    p[ipkt] = e->getPacket( pktid );
    //cout << "Found packet " << 2001+ipkt << "\t" << p[ipkt] << endl;

    if ( p[ipkt] )
    {
      for (int ich=0; ich<NCHPERPKT; ich++)
      {
        int feech = ipkt*NCHPERPKT + ich;
        //cout << feech << endl;
        for (int isamp=0; isamp<NSAMPLES; isamp++)
        {
          f_volt[feech][isamp] = p[ipkt]->iValue(isamp,ich);
          f_time[feech][isamp] = isamp;

          if ( f_volt[feech][isamp] <= 100 )
          {
            flag_err = 1;
            cout << "BAD " << f_evt << "\t" << feech << "\t" << f_time[feech][isamp]
                << "\t" << f_volt[feech][isamp] << endl;
          }
        }
      }

    }
    else
    {
      flag_err = 1;
      cout << "ERROR, evt " << f_evt << " Missing Packet " << pktid << endl;
    }

  }

  if ( flag_err==0 )
  {
    t->Fill();
  }
  else
  {
    cout << "Skipping t->Fill evt " << f_evt << endl;
  }

  // Delete the packets
  for (int ipkt=0; ipkt<MAX_PACKETS; ipkt++)
  {
    if ( p[ipkt]!=0 ) delete p[ipkt];
  }

  //CheckAlignment();

  return 0;

}
