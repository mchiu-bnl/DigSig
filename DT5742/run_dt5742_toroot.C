void run_dt5742_toroot(const int nevt)
{
  gROOT->ProcessLine(".L dt5742.C+");

  TString cmd = ".x dt5742_toroot.C("; cmd += nevt; cmd += ")";
  gROOT->ProcessLine( cmd.Data() );
}
