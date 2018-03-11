void run_ana_drs4(const char *fname, const int do_display = 0)
{
  //gSystem->Load("/data2/FTBF2016/DRS4/ANA/ana_drs4_C.so");
  gROOT->ProcessLine(".L ana_drs4.C+");
  ana_drs4(fname,do_display);
}
