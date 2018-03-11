// plot shapes from the shapes file
//
#include <iostream>
#include <fstream>
#include <TGraphErrors.h>

TGraphErrors* g_shape[576] = {0};

void plot_shapes(const char *basefname = "sig_gen0")
{
  int verbose = 1;
  int feech = 0;
  Int_t temp_ch = -9999;
  Int_t temp_nsamples;
  Double_t temp_begintime;
  Double_t temp_endtime;

  TString name = basefname; name += ".shape";
  cout << name << endl;
  ifstream shapefile(name);

  // Template 
  while ( shapefile >> temp_ch >> temp_nsamples >> temp_begintime >> temp_endtime )
    {
      g_shape[temp_ch] = new TGraphErrors(temp_nsamples);
      name = "shape"; name += temp_ch;
      g_shape[temp_ch]->SetName(name);
      g_shape[temp_ch]->SetTitle(name);
      if ( verbose ) cout << "shape " << temp_ch << "\t" <<  temp_nsamples << "\t" <<  temp_begintime << "\t" <<  temp_endtime << endl;

      Double_t step = (temp_endtime - temp_begintime)/(temp_nsamples-1);

      Double_t temp_val;
      for (int isamp=0; isamp<temp_nsamples; isamp++)
        {
          shapefile >> temp_val;

          g_shape[temp_ch]->SetPoint(isamp,temp_begintime+isamp*step,temp_val);

          if ( verbose )
          {
            cout << temp_val << " ";
            if ( isamp%10==9 ) cout << endl;
          }
        }
      if ( verbose ) cout << endl;

      g_shape[temp_ch]->Draw("ap");
      gPad->Modified();
      gPad->Update();
      /*
      string junk;
      cout << "ch" << temp_ch << ": ";
      cin >> junk;
      */
      gSystem->Exec("sleep 1");

    }
  
}

