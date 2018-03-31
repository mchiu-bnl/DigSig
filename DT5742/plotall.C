//
// Plot V vs t from all channels, integrated for all events
//
void plotall(const char *fname = "dt5742.root")
{
  TFile *tfile = new TFile(fname,"READ");
  TTree *t = (TTree*)tfile->Get("t");
  TCanvas *ac = new TCanvas("ac","signals",1600,800);
  ac->Divide(4,4);

  TString str;
  TString cut;
  for (int ich=0; ich<16; ich++)
  {
    ac->cd(ich+1);

    // 2d plot
    str = "ch"; str += ich; str += ":t"; str += ich;
    cut = "t"; cut += ich; cut += ">0"; cut += "&&t"; cut += ich; cut += "<8000&&evt<3000";
 
    // 1d plot
    //str = "ch"; str += ich; 

    //t->Draw(str,cut,"colz");
    t->Draw(str,cut);
  }
}

