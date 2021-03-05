// plot shapes from the shapes file
//
#include <iostream>
#include <fstream>
#include <TGraphErrors.h>

TGraphErrors* g_shape0[576] = {0};
TGraphErrors* g_shape1[576] = {0};
TGraphErrors* g_shape2[576] = {0};

void compare_shape()
{
  plot_shapes("sig_gen0");
  plot_shapes("sig_gen1");
  plot_shapes("sig_gen2");

  TCanvas *ac = new TCanvas("c_compare_shapes12","shape compare 14",550,425);
  g_shape0[12]->SetMarkerColor();
  g_shape0[12]->Draw();
  g_shape1[12]->SetMarkerColor(2);
  g_shape1[12]->SetLineColor(2);
  g_shape1[12]->SetLineWidth(2);
  g_shape1[12]->Draw("lsame");
  g_shape2[12]->SetMarkerColor(4);
  g_shape2[12]->SetLineColor(4);
  g_shape2[12]->Draw("lsame");

  TCanvas *bc = new TCanvas("c_compare_shapes14","shape compare 14",550,425);
  g_shape0[14]->Draw();
  g_shape1[14]->SetMarkerColor(2);
  g_shape1[14]->SetLineColor(2);
  g_shape1[14]->SetLineWidth(2);
  g_shape1[14]->Draw("lsame");
  g_shape2[14]->SetMarkerColor(4);
  g_shape2[14]->SetLineColor(4);
  g_shape2[14]->Draw("lsame");
}

// plot all the shapes from one file
void plot_shapes(const char *basefname = "sig_gen0")
{
  int verbose = 1;
  int feech = 0;
  int pass = 0;
  Int_t temp_ch = -9999;
  Int_t temp_nsamples;
  Double_t temp_begintime;
  Double_t temp_endtime;


  // find the pass
  TString name = basefname;
  if ( name == "sig_gen0" )
  {
    pass = 0;
  }
  else if ( name == "sig_gen1" )
  {
    pass = 1;
  }
  else if ( name == "sig_gen2" )
  {
    pass = 2;
  }

  cout << "Pass " << pass << endl;

  // Now get the shape file name
  name += ".shape";
  cout << name << endl;
  ifstream shapefile(name);

  // Template 
  while ( shapefile >> temp_ch >> temp_nsamples >> temp_begintime >> temp_endtime )
    {
      TGraphErrors *g_shape = new TGraphErrors(temp_nsamples);
      name = "shape"; name += temp_ch;
      g_shape->SetName(name);
      g_shape->SetTitle(name);
      if ( verbose ) cout << "shape " << temp_ch << "\t" <<  temp_nsamples << "\t" <<  temp_begintime << "\t" <<  temp_endtime << endl;

      Double_t step = (temp_endtime - temp_begintime)/(temp_nsamples-1);

      Double_t temp_val;
      for (int isamp=0; isamp<temp_nsamples; isamp++)
        {
          shapefile >> temp_val;

          g_shape->SetPoint(isamp,temp_begintime+isamp*step,temp_val);

          if ( verbose )
          {
            cout << temp_val << " ";
            if ( isamp%10==9 ) cout << endl;
          }
        }
      if ( verbose ) cout << endl;

      g_shape->Draw("ap");
      gPad->Modified();
      gPad->Update();

      if ( pass == 0 )
      {
        g_shape0[temp_ch] = g_shape;
      }
      else if ( pass == 1 )
      {
        g_shape1[temp_ch] = g_shape;
      }
      else if ( pass == 2 )
      {
        g_shape2[temp_ch] = g_shape;
      }

      /*
      string junk;
      cout << "ch" << temp_ch << ": ";
      cin >> junk;
      */

      //gSystem->Exec("sleep 1");

    }
  
}

