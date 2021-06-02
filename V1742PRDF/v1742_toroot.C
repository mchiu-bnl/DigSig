#include <pmonitor.h>
#include <v1742prdf.h>

R__LOAD_LIBRARY(libv1742prdf.so)

//void v1742_toroot(const char *prdfname, const char *chmap = "")
void v1742_toroot(const char *prdfname, const int nevents = 0)
{
  //SetChannelMapFile(chmap);
  pfileopen(prdfname);
  prun(nevents);

  SaveFile();
  //cleanup();
}

