//
//  ana_drs4.C
//
//  convert DRS4 data to root files
//

#include <iostream>
#include <fstream>
#include <fstream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cmath>

using namespace std;

const int N_CHN = 4;
const int NSAMPLES = 1024;

typedef struct {
   char ver_header[4]; // 'T' 'I' 'M' 'E'
   char time_header[4]; // 'T' 'I' 'M' 'E'
   char bn[2];          // 'B' '#'
   unsigned short board_number;
} THEADER;

typedef struct {
   char channel[4];     // eg, 'C' '0' '0' '1'
   float tcal[1024];
} TCHEADER;

typedef struct {
   char event_header[4];  // eg, 'E' 'H' 'D' 'R'
   unsigned int ev_serial_number;
   unsigned short year;
   unsigned short month;
   unsigned short day;
   unsigned short hour;
   unsigned short minute;
   unsigned short second;
   unsigned short millisec;
   unsigned short range;
   char bn[2];
   unsigned short board_number;
   char tc[2];
   unsigned short trigger_cell;
} EHEADER;

typedef struct {
   char channel[4];     // eg, 'C' '0' '0' '1'
   unsigned int scaler;
   unsigned short data[NSAMPLES];
} CHANNEL;

/*-----------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
   int ch, i, j, fh, ndt;
   double threshold, t1, t2, dt, sumdt, sumdt2;
   THEADER th;
   TCHEADER tch[N_CHN];
   EHEADER eh;
   CHANNEL channel[N_CHN];

   // output info
   int f_evt;
   unsigned short f_cell;  // trigger cell
   float waveform[N_CHN][NSAMPLES];
   float time[N_CHN][NSAMPLES];
   
   const int do_check = 0;
   int nevents = 0;

   if ( argc != 2 )
   {
     cout << "ERROR, usage: drs4tocsv <filename.dat>" << endl;
     return 1;
   }

   const char *fname = argv[1];

   // open input file
   fh = open(fname, O_RDONLY, 0666);
   if (fh < 0) {
      printf("Cannot find data file %s\n",fname);
      return 0;
   }

   // read time header (one set of calibs per file)
   read(fh, &th, sizeof(th));
   if ( do_check ) {
     cout << "board no. " << th.board_number << endl;
   }
   // read time bin widths (tcal)
   for (ch=0 ; ch<N_CHN ; ch++) {
     read(fh, &tch[ch], sizeof(tch[ch]));
     if ( do_check ) {
       cout << "====  channel " << ch << endl;
       for (int i=0; i<NSAMPLES; i++)
       {
         cout << tch[ch].tcal[i] << "\t";
         if ( i%8==7 ) cout << endl;
         tch[ch].tcal[i] = 1.5;
       }
     }
   }

   // open the output csv file
   string savefname(fname);
   int len = savefname.size();
   savefname.replace(len-3,3,"csv");
   ofstream savefile(savefname.c_str());
   cout << "Saving to " << savefname << endl;

   // Now loop over events
   for (int nevt=0; nevt<10; nevt++) {
      // read event header
      i = (int)read(fh, &eh, sizeof(eh));

      f_evt = nevt+1;
      f_cell = eh.trigger_cell;

      if (do_check)
      {
        cout << "evt\t" << eh.ev_serial_number << endl;
        cout << "range\t" << eh.range << endl;
        cout << "board\t" << eh.board_number << endl;
        cout << "trigcell\t" << eh.trigger_cell << endl;
      }

      // check for valid event header
      if (memcmp(eh.event_header, "EHDR", 4) != 0) {
         printf("Invalid event header (probably number of saved channels not equal %d)\n", N_CHN);

         return 0;
      }

      //cout << "Range = " << eh.range << endl;

      // print notification every 100 events
      if (nevt % 100 == 0 || i != sizeof(eh))
         printf("Analyzing event #%d\n", nevt);
      
      // stop if end-of-file
      if (i != sizeof(eh))
         break;
      
      // read channel data
      for (int ch=0 ; ch<N_CHN ; ch++)
      {

        read(fh, &channel[ch], sizeof(channel[ch]));

        // check for valid channel header
        if (memcmp(channel[ch].channel, "C00", 3) != 0) {
          printf("Invalid channel header  %d\n", ch);
          return 0;
        }

        for (int i=0 ; i<NSAMPLES ; i++)
        {
          // convert to volts
          // this is the older version
          //waveform[ch][i] = (channel[ch].data[i]/65536.0-0.5)*1000.;
          // this is what is used in the latest code (v5.0.4)
          waveform[ch][i] = (channel[ch].data[i] / 65536. + eh.range/1000.0 - 0.5);

          // calculate time for this cell
          time[ch][i] = 0;
          for (int j=0; j<i ; j++)
          {
            time[ch][i] += tch[ch].tcal[(j+eh.trigger_cell) % NSAMPLES];
          }
        }
      }

      //cout << "Aligning cells" << endl;

      // align cell #0 of all channels
      t1 = time[0][(NSAMPLES-eh.trigger_cell) % NSAMPLES];
      for (int ch=1 ; ch<N_CHN ; ch++) {
        t2 = time[ch][(NSAMPLES-eh.trigger_cell) % NSAMPLES];
        dt = t1 - t2;
        for (int i=0 ; i<NSAMPLES ; i++)
        {
          time[ch][i] += dt;
        }
      }

      // now write it out to savefile as csv
      for (int ch=0; ch<N_CHN ; ch++) {
        for (int isamp=0 ; isamp<NSAMPLES ; isamp++) {
          savefile << time[ch][isamp] << "," << waveform[ch][isamp] << endl;
        }
      }
  
      nevents++;

   }
 
   // save and close csv file
   savefile.close();

   cout << "Processed " << nevents << endl;

   return 0;
}

