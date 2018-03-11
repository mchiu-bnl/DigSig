#ifndef __PSTOF_DT5742__
#define __PSTOF_DT5742__

#include <fstream>
#include <iostream>
#include <TGraph.h>
#include <TGraphErrors.h>



//const int MAXCH = 16;
const int MAXCH = 1;
//const int MAXCH = 14;
//const int MAXCH = 6;
const int NSAMPLES = 1024;
const int NUM_PMT = 1;
float adc[MAXCH][NSAMPLES];
TGraphErrors *trace[MAXCH];         // adc vs time
float max_ampl[MAXCH];        // max amplitude for each channel in an event
float min_ampl[MAXCH];        // min amplitude for each channel in an event
int   max_samp[MAXCH];        // sample num of max amplitude
int   min_samp[MAXCH];        // sample num of min amplitude
float max_time[MAXCH];        // time of max amplitude
float min_time[MAXCH];        // time of min amplitude

ifstream wavefile[MAXCH];
int g_record_len;  // number of samples
int g_board_id;    // board id
int g_channel;     // channel
int g_event_num;   // event number (starting from 0!)
int g_pattern;     // pattern?
UInt_t g_time_stamp;  // time stamp?
int g_dc_offset;   // DC offset
int g_start_cell;  // starting cell of readout

// text
const float DATA_TIMESTEP = 0.2;  // in nanoseconds
const int   DIGI_STEPS = 4000;      // number of steps in template
const float DIGI_START_TIME = -20.;
const float DIGI_END_TIME = 20.;
const float DIGI_TIMESTEP = (DIGI_END_TIME - DIGI_START_TIME)/DIGI_STEPS;
TGraph *digi_graph[MAXCH];
float digi_pulse[MAXCH][DIGI_STEPS] = {{0.}};


Double_t FitFcn(Double_t *x, Double_t *par);
void set_fitpulse(const int ich);
//double getpedestal(TGraphErrors *g);

#endif  // __PSTOF_DT5742__
