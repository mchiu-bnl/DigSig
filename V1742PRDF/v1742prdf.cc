
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <pmonitor/pmonitor.h>
#include "v1742prdf.h"

#include <TFile.h>
#include <TTree.h>
#include <TString.h>
//#include <TH1.h>

int init_done = 0;

using namespace std;

const int MAX_PACKETS = 4; // Maximum number of packets (ie, boards)
const int MAX_DRS = 4;     // Maximum number of DRS4 per board
const int MAX_CALCH = 9;   // Maximum number of ch per DRS4
const int NCH = 2;         // number active channels
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

Float_t ped_cell[MAX_PACKETS][MAX_DRS][MAX_CALCH][1024]; // [board][drs][ch][cell]
Float_t nsamp_cell[MAX_PACKETS][MAX_DRS][MAX_CALCH][1024]; // [board][drs][ch][cell]
Float_t time_cell[MAX_PACKETS][MAX_DRS][1024]; // [board][drs][cell]

void LoadCorrections()
{
  string FullLine;

  ifstream infile;

  TString caldir = "Corrections/";
  TString calfname;

  for (int iboard=0; iboard<MAX_PACKETS; iboard++)
  {
    for (int idrs=0; idrs<MAX_DRS; idrs++)
    {

      calfname.Clear();
      calfname = caldir;
      calfname += iboard; calfname += "_gr";
      calfname += idrs; calfname += "_cell.txt";
      cout << calfname << endl;

      infile.open( calfname.Data() );

      // Read in pedestals (cell)
      while ( !infile.eof() )
      {

        for (int ich=0; ich<MAX_CALCH; ich++)
        {
          // skip first two lines
          getline(infile, FullLine);
          //cout << FullLine << endl;
          getline(infile, FullLine);

          const int NLINES = 1024/8;  // num lines in calib file for each ch
          for (int iline=0; iline<NLINES; iline++)
          {
            getline(infile, FullLine);
            // make FullLine an istringstream
            istringstream line( FullLine.c_str() );

            for (int icell=0; icell<8; icell++)
            {
              line >> ped_cell[iboard][idrs][ich][iline*8+icell];
              //cout << ich << "\t" << iline*8+icell << "\t" << ped_cell[0][0][ich][iline*8+icell] << endl;
            }
          }
        }
      }  
      infile.close();

      // Read in nsamples
      calfname.Clear();
      calfname = caldir;
      calfname += iboard; calfname += "_gr";
      calfname += idrs; calfname += "_nsample.txt";
      cout << calfname << endl;

      infile.open( calfname.Data() );

      while ( !infile.eof() )
      {
        for (int ich=0; ich<9; ich++)
        {
          // skip first two lines
          getline(infile, FullLine);
          //cout << FullLine << endl;
          getline(infile, FullLine);

          const int NLINES = 1024/8;  // num lines in calib file for each ch
          for (int iline=0; iline<NLINES; iline++)
          {
            getline(infile, FullLine);
            // make FullLine an istringstream
            istringstream line( FullLine.c_str() );

            for (int icell=0; icell<8; icell++)
            {
              line >> nsamp_cell[iboard][idrs][ich][iline*8+icell];
              //cout << ich << "\t" << iline*8+icell << "\t" << nsamp_cell[0][0][ich][iline*8+icell] << endl;
            }
          }
        }
      }  
      infile.close();

      // Read in time corrections (only one set per drs)
      calfname.Clear();
      calfname = caldir;
      calfname += iboard; calfname += "_gr";
      calfname += idrs; calfname += "_time.txt";
      cout << calfname << endl;

      infile.open( calfname.Data() );

      while ( !infile.eof() )
      {
        // skip first two lines
        getline(infile, FullLine);
        //cout << FullLine << endl;
        getline(infile, FullLine);

        const int NLINES = 1024/8;  // num lines in calib file for each ch
        for (int iline=0; iline<NLINES; iline++)
        {
          getline(infile, FullLine);
          // make FullLine an istringstream
          istringstream line( FullLine.c_str() );

          for (int icell=0; icell<8; icell++)
          {
            line >> time_cell[iboard][idrs][iline*8+icell];
            //cout << ich << "\t" << iline*8+icell << "\t" << time_cell[0][0][iline*8+icell] << endl;
          }
        }
      }  
      infile.close();

    } // idrs
  } // iboard


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
  if ( savefile )
  {
    savefile->Write();
  }

  return 0;
}

int pinit()
{

  if (init_done) return 1;
  init_done = 1;

  TString name;
  TString leaflist;

  savefile = new TFile("v1742.root","RECREATE");

  /*
  for (int ich=0; ich<MAXCH; ich++)
  {
    name = "hadc"; name += ich;
    hadc[ich] = new TH1D(name,name,4096,-0.5,4095.5);
    hadc[ich]->SetXTitle("ADC Pedestal");

    name = "htrace_flipped"; name += ich;
    htrace_flipped[ich] = new TH1F(name,name,30*5,0.,30.);
    htrace_flipped[ich]->SetXTitle("t (ns)");

    name = "hprof_adc"; name += ich;
    hprof_adc[ich] = new TProfile(name,name,1024,-0.5,1023.5,-0.5,4095.5);
  }
  */

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
  Packet *p[MAX_PACKETS] = {0};

  // Get packets, but require packet 2000
  for (int iboard=0; iboard<MAX_PACKETS; iboard++)
  {
    p[iboard] = e->getPacket(2000+iboard);
    //cout << "Fount packet " << 2000+iboard << "\t" << p[iboard] << endl;
  }

  if ( p[0] )
  {
    f_evt = p[0]->iValue(0,"EVNR");
    if ( f_evt%1000 == 0 )
    {
      cout << "Event " << f_evt << endl;
    }

    /*
    // Dump for debugging
    p[0]->dump();
    string junk;
    cout << "? ";
    cin >> junk;
    */
  }
  else
  {
    cout << "Packet 2000 NOT FOUND " << endl;
    return 0;
  }

  /*
  // For debugging
  int s1 = p[0]->iValue(100, 0);
  int s2 = p[0]->iValue(100, 8);
  cout << f_evt << "\t" << s1 << "\t" << s2 << endl;
  */

  for ( int ich = 0; ich < NCH; ich++)
  {
    int feech = chmap[ich];
    int board = ich/32;
    int drs = (chmap[ich]%32)/8;
    int channel = feech%8;

    //cout << "AAA " << board << "\t" << channel << endl;
    int trigcell = p[board]->iValue(drs,"INDEXCELL");

    if ( f_evt == 1 )
    {
      cout << "ch " << feech << "\t" << board << "\t" << drs << "\t" << channel << endl;
    }

    for ( int isamp = 0; isamp < 1024; isamp++)
    {
      int s = p[board]->iValue(isamp, chmap[ich]%32);

      // pedestal correction
      int corr_sample = (trigcell+isamp)%1024;
      s -= ped_cell[board][drs][channel][corr_sample];
      s -= nsamp_cell[board][drs][channel][corr_sample];

      f_time[ich][isamp] = time_cell[board][drs][isamp];
      f_volt[ich][isamp] = s;
    }

  }
  t->Fill();

  // Delete the packets
  for (int ipkt=0; ipkt<MAX_PACKETS; ipkt++)
  {
    if ( p[ipkt]!=0 ) delete p[ipkt];
  }

  return 0;

}

