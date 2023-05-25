#include <pmonitor.h>
//#include "digsig_prdf.h"

int SaveFile();
void SetSaveFileName(const char *);

R__LOAD_LIBRARY(libdigsig_prdf.so)

void prdf2root(const char *prdfname, const int nevents = 0)
{
  TString name = prdfname;
  name.ReplaceAll(".prdf",".root");
  int index = name.Last('/');
  if ( index > 0 )
  {
    name.Remove(0,index+1);
  }

  SetSaveFileName( name );
  cout << "Writing to " << name << endl;

  pfileopen(prdfname);
  prun(nevents);

  SaveFile();
  //cleanup();
}

