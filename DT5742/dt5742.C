// read in ascii files from CAEN DT5742 wavedump output
//
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TF1.h>
#include <TString.h>

#include "dt5742.h"

// Open each wave_NN.txt or .dat file
// binorasc: 0=text, 1=binary
// Each wave file corresponds to 1 channel
int OpenWaveFiles(const int binorasc)
{
  TString name;
  int maxch = MAXCH;

  if ( MAXCH==17 ) // digitize TR0 channel
  {
    maxch = 16;
    // open up wave files
    // (should maybe include TR_x_x.txt/dat files)
    if ( binorasc==0 )
    {
      name = "TR_0_0.txt";
      wavefile[maxch].open( name.Data() );
      //cout << "Opening trigger file TR_0_0.txt" << endl;
    }
    else
    {
      name = "TR_0_0.dat";
      wavefile_fp[maxch] = fopen( name.Data(), "rb" );
    }

    // make trace graphs of each channel in the event
    trace[maxch] = new TGraphErrors();
    name = "trace"; name += maxch;
    trace[maxch]->SetName( name );
    trace[maxch]->SetMarkerColor( maxch%8+1 );
    trace[maxch]->SetLineColor( maxch%8+1 );
  }

  for (int ich=0; ich<maxch; ich++)
  {
    // open up wave files
    // (should maybe include TR_x_x.txt/dat files)
    if ( binorasc==0 )
    {
      name = "wave_"; name += ich; name += ".txt";
      wavefile[ich].open( name.Data() );
    }
    else
    {
      name = "wave_"; name += ich; name += ".dat";
      wavefile_fp[ich] = fopen( name.Data(), "rb" );
    }

    // make trace graphs of each channel in the event
    trace[ich] = new TGraphErrors();
    name = "trace"; name += ich;
    trace[ich]->SetName( name );
    trace[ich]->SetMarkerColor( ich%8+1 );
    trace[ich]->SetLineColor( ich%8+1 );
    //if ( ich==9 ) trace[ich]->SetMarkerColor( 2 );
    trace[ich]->SetMarkerStyle( 20 );
    trace[ich]->SetMarkerSize( 0.5 );
    //trace[ich]->SetMaximum( 100. );
    //trace[ich]->SetMinimum( -500. );
    //trace[ich]->SetMaximum( 20. );
    //trace[ich]->SetMinimum( -200. );
  }

  return 1;
}

int CloseWaveFiles(const int binorasc)
{
  for (int ich=0; ich<MAXCH; ich++)
  {
    if ( binorasc == 0 )
    {
      wavefile[ich].close();
    }
    else
    {
      fclose( wavefile_fp[ich] );
    }
  }

  return 1;
}

int ReadSingleEventAscii()
{
  string FullLine;
  string junk1, junk2, junk3;
  istringstream line;
  g_record_len = 1024;  // number of samples
  g_board_id = 0;    // board id
  g_channel = 0;     // channel
  g_event_num = -1;  // event number (starting from 0!)
  g_pattern = -1;    // pattern?
  g_time_stamp = 0; // time stamp?
  g_dc_offset = -1;  // DC offset
  g_start_cell = -1; // starting cell of readout

  for (unsigned int ich=0; ich<MAXCH; ich++)
    {
      max_ampl[ich] = 0;
      min_ampl[ich] = 5000;
      min_samp[ich] = -999;
      max_samp[ich] = -999;
      min_time[ich] = -999.;
      max_time[ich] = -999.;

      // get one line first 
      getline(wavefile[ich], FullLine);
      if ( wavefile[ich].eof() ) return 0;
      if ( wavefile[ich].bad() ) return 0;
      if ( ! wavefile[ich].good() ) return 0;

      //cout << "zzz " << FullLine << endl;

      // skip lines that begin with #, or "//"
      while ( FullLine[0]=='#' || FullLine.substr(0, 2) == "//" )
        {
          getline(wavefile[ich],FullLine);
        }

      // Check for header
      int start_samp = 0;
      if ( FullLine[0]=='R' )
        {
          // We have a header
          //
          // make FullLine an istringstream
          line.clear();
          line.str( FullLine.c_str() );
          line >> junk1 >> junk2 >> g_record_len;
          //cout << "Record Len " << g_record_len << endl;

          getline(wavefile[ich],FullLine);
          line.clear();
          line.str( FullLine.c_str() );
          line >> junk1 >> g_board_id;
          //cout << "Bd ID " << g_board_id << endl;

          getline(wavefile[ich],FullLine);
          line.clear();
          line.str( FullLine.c_str() );
          line >> junk1 >> junk2;
          if ( junk2 == "TR_0_0" )
            {
              g_channel = 17;
            }
          else if ( junk2 == "TR_0_1" )
            {
              g_channel = 18;
            }
          else
            {
              g_channel = atoi(junk2.c_str());
            }
          // note, there is a bug in the wavedump where the TR_0_0.txt file reports it is ch 8.
          if ( g_channel != ich && ich != 16 )
            {
              cout << "ERROR, g_channel and ich are " << g_channel << " " << ich << endl;
            }
          //cout << "Channel " << g_channel << endl;

          getline(wavefile[ich],FullLine);
          line.clear();
          line.str( FullLine.c_str() );
          line >> junk1 >> junk2 >> g_event_num;
          //cout << "Event Num " << g_event_num << endl;

          getline(wavefile[ich],FullLine);
          line.clear();
          line.str( FullLine.c_str() );
          line >> junk1 >> g_pattern;

          getline(wavefile[ich],FullLine);
          line.clear();
          line.str( FullLine.c_str() );
          line >> junk1 >> junk2 >> junk3 >> g_time_stamp;

          getline(wavefile[ich],FullLine);
          line.clear();
          line.str( FullLine.c_str() );
          line >> junk1 >> junk2 >> junk3 >> hex >> g_dc_offset >> dec;

          getline(wavefile[ich],FullLine);
          line.clear();
          line.str( FullLine.c_str() );
          line >> junk1 >> junk2 >> junk3 >> g_start_cell;
          //cout << "Starting cell " << g_start_cell << endl;
        }
      else
      {
        line.clear();
        line.str( FullLine.c_str() );
        line >> adc[ich][0];
        //cout << adc[ich][0] << endl;
        start_samp = 0;
      }

      //trace[ich]->Reset();
      for (uint32_t isamp=start_samp; isamp<g_record_len; isamp++)
      {
        getline(wavefile[ich],FullLine);
        line.clear();
        line.str( FullLine.c_str() );
        line >> adc[ich][isamp];
        //if (isamp==NSAMPLES-1) cout << "\t" << adc[ich][isamp] << endl;
        float time = isamp*DATA_TIMESTEP; // DATA_TIMESTEP is in ns
        trace[ich]->SetPoint(isamp,time,adc[ich][isamp]);
        trace[ich]->SetPointError(isamp,0.,1.0);  // should change the error

        if ( max_ampl[ich] < adc[ich][isamp] )
        {
          max_ampl[ich] = adc[ich][isamp];
          max_samp[ich] = isamp;
          max_time[ich] = time;
        }
        if ( min_ampl[ich] > adc[ich][isamp] )
        {
          min_ampl[ich] = adc[ich][isamp];
          min_samp[ich] = isamp;
          min_time[ich] = time;
        }
      }
    }

  return 1;
}


int ReadSingleEventBinary()
{
  //  string FullLine;
  //  string junk1, junk2, junk3;
  //  istringstream line;
  const int verbose = 0;

  g_record_len = 0;  // number of samples
  g_board_id = 0;    // board id
  g_channel = 0;     // channel
  g_event_num = -1;  // event number (starting from 0!)
  g_pattern = -1;    // pattern?
  g_time_stamp = 0; // time stamp?
  g_dc_offset = -1;  // DC offset
  g_start_cell = -1; // starting cell of readout

  for (int ich=0; ich<MAXCH; ich++)
  {
    max_ampl[ich] = 0;
    min_ampl[ich] = 5000;
    min_samp[ich] = -999;
    max_samp[ich] = -999;
    min_time[ich] = -999.;
    max_time[ich] = -999.;

    // get one line first 
    size_t nread = fread(&g_record_len,sizeof(uint32_t),1,wavefile_fp[ich]);
    if ( nread != 1 )
    {
      return 0;
    }
    fread(&g_board_id,sizeof(uint32_t),1,wavefile_fp[ich]);
    fread(&g_pattern,sizeof(uint32_t),1,wavefile_fp[ich]);
    fread(&g_channel,sizeof(uint32_t),1,wavefile_fp[ich]);
    fread(&g_event_num,sizeof(uint32_t),1,wavefile_fp[ich]);
    fread(&g_time_stamp,sizeof(uint32_t),1,wavefile_fp[ich]);

    if ( verbose>0 )
    {
      cout << "*** CH "  << ich << endl;
      cout << "len\t"    << g_record_len << endl;
      cout << "board\t"  << g_board_id << endl;
      cout << "patt\t"   << g_pattern << endl;
      cout << "ch\t"     << g_channel << endl;
      cout << "evt\t"    << g_event_num << endl;
      cout << "tstamp\t" << g_time_stamp << endl;
    }

    /*
       if ( g_channel != ich%8 )
       {
       cout << "ERROR, " << g_channel << " != " << ich << endl;
       }
       */

    /*
       if ( ich==16 )
       {
       cout << "16 16 16" << endl;
       }
       */
    //trace[ich]->Reset();
    for (int isamp=0; isamp<1024; isamp++)
    {
      size_t nread = fread(&adc[ich][isamp],sizeof(float),1,wavefile_fp[ich]);
      if ( nread != 1 )
      {
        return 0;
      }

      float time = isamp*DATA_TIMESTEP; // DATA_TIMESTEP is in ns
      trace[ich]->SetPoint(isamp,time,adc[ich][isamp]);
      trace[ich]->SetPointError(isamp,0.,1.0);  // should change the error

      /*
         if ( ich==16 )
         {
         cout << isamp << "\t" << time << "\t" << adc[ich][isamp] << endl;
         }
         */

      if ( max_ampl[ich] < adc[ich][isamp] )
      {
        max_ampl[ich] = adc[ich][isamp];
        max_samp[ich] = isamp;
        max_time[ich] = time;
      }
      if ( min_ampl[ich] > adc[ich][isamp] )
      {
        min_ampl[ich] = adc[ich][isamp];
        min_samp[ich] = isamp;
        min_time[ich] = time;
      }
    }
  }

  return 1;
}


