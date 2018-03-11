// read in ascii files from CAEN DT5742 wavedump output
//
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TF1.h>
#include <TString.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "dt5742.h"

// should put these in a separate library macro

// return pedestal
// (should remove outliers)
double getpedestal(TGraphErrors *g)
{
  // go up to t=20 ns to determine pedestal
  //const int nsamp = g->GetHistogram()->FindBin( 20. );
  //const int last_samp = 50;
  const int last_samp = 20;
  double nsamp = 0.;
  double sum = 0.;
  for (int isamp=10; isamp<last_samp; isamp++)
    {
      Double_t t,v;
      g->GetPoint(isamp,t,v);
      //cout << isamp << "\t" << t << "\t" << v << endl;
      sum += v;
      nsamp += 1.0;
    }
  sum /= nsamp;

  //cout << "ped " << sum << endl;
  return sum;
}


// return amplitude, simple max (or min if negative)
// assumes negative, should modify so that amp gives sign
void getamplitude(TGraph *g, double& amp, int& maxsamp)
{
  //cout << "in getamplitude()" << endl;
  maxsamp = -1;
  amp = 9999.;
  // skip first 10 and last 20 since they are usually bad
  for (int isamp=10; isamp<1000; isamp++)
    {
      Double_t t,v;
      g->GetPoint(isamp,t,v);
      if ( v < amp )
        {
          amp = v;
          maxsamp = isamp;
        }
    }

  //cout << amp << "\t" << maxsamp << endl;
}

// return amplitude, simple max (or min if negative)
double getintegral(TGraph *g, double min, double max)
{
  int minsamp = min/0.2; // quick and dirty way to get sample
  int maxsamp = max/0.2;

  double sum = 0.;

  // now do integral \int{Vdt}
  Double_t t0,v0;
  Double_t t1,v1;
  for (int isamp=minsamp; isamp<maxsamp; isamp++)
    {
      g->GetPoint(isamp,t0,v0);
      g->GetPoint(isamp+1,t1,v1);

      double v = (v0+v1)/2.0;
      sum += v*(t1-t0);
    }

  return sum;
}

// Find time at midpoint
double gettime(TGraph *g, double amp, int maxsamp)
{
  double start_time = -9999;

  // check sign
  /*
  double sign = 1;
  if ( amp<0 ) sign = -1;
  */

  for (int isamp=maxsamp; isamp>=0; isamp--)
  {
    Double_t t1,v1;
    g->GetPoint(isamp,t1,v1);
    if ( v1 > (amp/2.0) )
    {
      Double_t t2,v2;
      g->GetPoint(isamp+1,t2,v2);
      // interpolate time
      start_time = (amp/2.0-v1)*(t2-t1)/(v2-v1)+t1;
      //cout << "nch " << nch << "\t" << start_time[nch] << endl;
      break;
    }
  }

  return start_time;
}

// Find time at (neg) threshold between tmin and tmax (tmin,tmax are in ns)
double gettime(TGraph *g, double thresh, double tmin, double tmax)
{
  double start_time = -9999.;
  // quick and dirty way to get sample number, since each sample is ~200 ps
  int begin_samp = static_cast<int>(tmin/0.2);
  int end_samp = static_cast<int>(tmax/0.2);

  for (int isamp=begin_samp; isamp<end_samp; isamp++)
    {
      Double_t t1,v1;
      g->GetPoint(isamp,t1,v1);
      if ( v1 < thresh )
        {
          Double_t t0,v0;
          g->GetPoint(isamp-1,t0,v0);
          // interpolate time
          double slope = (v1-v0)/(t1-t0);
          if ( slope!= 0 )
          {
            start_time = t0 + (thresh - v0)/slope;
          }
          else
          {
            cout << "ERROR in gettime(), v1==v0" << endl;
            return -2.0;
          }

          //cout << "stime\t" << start_time << endl;
          break;
        }
    }

  return start_time;
}


// Find time at (neg) threshold between tmin and tmax (tmin,tmax are in ns)
double gettimeCFD(TGraph *g, double fraction, int delay, double tmin, double tmax)
{
  double start_time = -9999.;
  // quick and dirty way to get sample number, since each sample is ~200 ps
  int begin_samp = static_cast<int>(tmin/0.2);
  int end_samp = static_cast<int>(tmax/0.2);

  // Generate v'_i = fraction*v_i - v_(i-delay)
  Int_t n = 0;
  Double_t t[1024], v[1024];
  Double_t t1, v1;
  Double_t t2, v2;
  for (int isamp=begin_samp; isamp<end_samp; isamp++)
  {
    if ( isamp+delay>1023 ) break;
    g->GetPoint(isamp,t1,v1);
    g->GetPoint(isamp+delay,t2,v2);

    t[n] = t2;
    v[n] = fraction*v2 - v1;
    n++;
  }

  TGraph g_cfd(n,t,v);
  g_cfd.Draw("ap");
  cout << "in gettimeCFD()" << endl;
  string junk;
  cin >> junk;

  return start_time;
}

// Open each wave_NN.txt file
// Each wave file corresponds to 1 channel
int OpenWaveFiles()
{
  TString name;
  for (int ich=0; ich<MAXCH; ich++)
  {
    // open up wave files
    name = "wave_"; name += ich; name += ".txt";
    wavefile[ich].open( name.Data() );

    // make trace graphs of each channel in the event
    trace[ich] = new TGraphErrors();
    name = "trace"; name += ich;
    trace[ich]->SetName( name );
    trace[ich]->SetMarkerColor( ich%8+1 );
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

int CloseWaveFiles()
{
  for (int ich=0; ich<MAXCH; ich++)
    {
      wavefile[ich].close();
    }

  return 1;
}

int ReadSingleEvent()
{
  string FullLine;
  string junk1, junk2, junk3;
  istringstream line;
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
      getline(wavefile[ich], FullLine);
      if ( wavefile[ich].eof() ) return 0;
      if ( wavefile[ich].bad() ) return 0;

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
          line >> junk1 >> g_channel;
          //cout << junk1 << "\t" << g_channel << endl;
          if ( g_channel != ich )
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
          cout << adc[ich][0];
          start_samp = 1;
        }

      //trace[ich]->Reset();
      for (int isamp=start_samp; isamp<g_record_len; isamp++)
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


