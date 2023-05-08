//
// Convert Chi's *.dat files from prototype testing
// The file starts with three values:
//   nsteps
//   nevents/step
//   time_delay step
//
// Then there is a start marker (12ffff)
// Followed by the header word
// then the data!
//
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

// Should change to read in the no. of ADC boards from config file
const int MAXADC = 4;           // Max Possible ADC Boards 
const int NCH_PER_BOARD = 64;   // Num Ch in ADC Board
const int NSAMPLES = 20;
//const int NSAMPLES = 30;

int NADC = 1;   // Num ADC Boards 
int NCH = 64;   // Num Ch total
//const int NSAMPLES = 12;

TFile *savefile;
TTree *t;
TH1 *h_pars;  // histogram with calibration run parameters taken from beginning of dat file
Int_t f_evt;
Float_t f_adc[MAXADC*NCH_PER_BOARD][NSAMPLES];
Float_t f_samp[NSAMPLES];

void read_mbd(const char *fname = "test_odd.dat")
{
  int verbose = 0;  // sets level of screen output

  ifstream configfile("digsig.cfg");
  if ( configfile.is_open() )
  {
    string junk;
    configfile >> junk >> NCH;
    NADC = NCH/64;

    cout << "Found config file digsig.cfg" << endl;
    cout << "Setting NCH = " << NCH << endl;
    cout << "Setting NADC = " << NADC << endl;
  }


  // fill sample numbers
  for (int isamp=0; isamp<NSAMPLES; isamp++)
  {
    f_samp[isamp] = isamp;
  }

  ifstream datfile(fname);

  unsigned int raw_data[20000] = {0};
  for (int i=0; i<20000; i++)
  {
    raw_data[i] = 0xa5a5;
  }

  f_evt = 0;
  int idx = 0;

  // skip 1st three lines (format for tpdelay files)
  int ndelays;
  int nevents_per_delay;
  int delay_step;

  TString t_fname = fname;
  if ( t_fname.EndsWith(".dat2") )
  { 
    datfile >> ndelays;
    datfile >> nevents_per_delay;
    datfile >> delay_step;
  }

  h_pars = new TH1I("h_pars","Calib Run Parameters",5,0,5);
  h_pars->Fill(0., ndelays);
  h_pars->Fill(1., nevents_per_delay);
  h_pars->Fill(2., delay_step);
  h_pars->Fill(3., NADC);
  h_pars->Fill(4., NCH);

  // Start Processing Chi's dat file (event loop over data)
  unsigned int marker;
  unsigned int header;
  unsigned int trailer;
  int bad_evt = 0;
  while ( datfile >> hex >> marker )
  {
    if ( verbose ) cout << "marker " << hex << marker << dec << endl;

    if ( (marker&0xffffffff) != 0x3ffff )
    {
      cout << "ERROR in marker, " << hex << marker << dec << endl;
      bad_evt = 1;
    }
    else
    {
      bad_evt = 0;
    }

    // loop over adc boards
    for (int iadc = 0; iadc<NADC; iadc++)
    {
      // See if this is the 1st ADC board, which marks a new event
      //if (raw_data[idx] == 0x11ffff) 
      if ( iadc == 0 )
      {
        if ( bad_evt==0 ) f_evt++;

        //if ( f_evt == 10 ) return;

        if ( nevents_per_delay!=0 )
        {
          if ( f_evt%nevents_per_delay == 0 ) cout << "Event " << f_evt << endl;
        }
        else if ( f_evt%100 == 0 )
        {
          cout << "Event " << f_evt << endl;
        }

        if ( verbose ) cout << hex << raw_data[idx] << endl;
      }
      else
      {
        // get marker for next board
        datfile >> marker;
        if ( verbose ) cout << "Board " << dec << iadc << " marker " << hex << marker << endl;
      }

      datfile >> header;  // header word
      //datfile >> hex >> raw_data[++idx];  // header word
      if ( verbose ) cout << hex << header << endl;

      for (int ich=0; ich<NCH; ich+=2)
      {
        for (int isamp=0; isamp<NSAMPLES; isamp++)
        {
          datfile >> raw_data[++idx];

          f_adc[iadc*NCH+ich][isamp] = raw_data[idx]&0x3fff;
          f_adc[iadc*NCH+ich+1][isamp] = ((raw_data[idx] >> 16) & 0x3fff);
        }
      }

      datfile >> hex >> trailer;  // end word of ADC Board packet
      if ( verbose ) cout << "trailer: " << hex << raw_data[idx] << dec << endl;
    }

    // Print ADC by ch
    if ( verbose )
    {
      for (int ich=0; ich<NADC*NCH; ich++)
      {
        cout << dec << ich << ":\t";
        for (int isamp=0; isamp<NSAMPLES; isamp++)
        {
          cout << hex << f_adc[ich][isamp] << " ";
        }
        cout << endl;
      }
    }

    if ( verbose ) cout << "done with event " << dec << f_evt << endl;

    idx = 0;  // reset so that raw_data starts fresh
    if ( bad_evt==0 ) t->Fill();

    if ( verbose )
    {
      string junk;
      cin >> junk;
      if ( junk[0] == 'q' ) break;
    }

  } // while loop over Chi's dat file

  cout << "Done!" << endl;
  datfile.close();
}


void mbd_toroot(const char *fname = "test_odd.dat")
{
  TString name;
  TString leaflist;

  // Replace dot extension with .root
  name = fname;
  int index = name.Last('.');
  if ( index >= 0 )
  {
    name.Remove(index);
  }
  name.Append(".root");
  cout << name << endl;

  savefile = new TFile(name,"RECREATE");
  t = new TTree("t","MBD data");
  t->Branch("evt",&f_evt,"evt/I");
  for (int ich=0; ich<NADC*NCH; ich++)
  {
    name = "ch"; name += ich;
    leaflist = name; leaflist += "["; leaflist += NSAMPLES; leaflist += "]/F";
    t->Branch(name,f_adc[ich],leaflist);    // adc value

    name = "t"; name += ich;
    leaflist = name; leaflist += "["; leaflist += NSAMPLES; leaflist += "]/F";
    t->Branch(name,f_samp,leaflist);    // sample number
  }

  read_mbd(fname);

  savefile->Write();
  //savefile->Close();
}

