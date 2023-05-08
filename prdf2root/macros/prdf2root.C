#include <pmonitor.h>
//#include "digsig_prdf.h"

int SaveFile();

R__LOAD_LIBRARY(libdigsig_prdf.so)

void prdf2root(const char *prdfname, const int nevents = 0)
{
  pfileopen(prdfname);
  prun(nevents);

  SaveFile();
  //cleanup();
}

