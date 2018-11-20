
void v1742_toroot(const char *prdfname, const char *chmap = "")
{
  gSystem->Load("libv1742prdf.so");
  SetChannelMapFile(chmap);
  pfileopen(prdfname);
  prun(5000);

  SaveFile();
  //cleanup();
}

