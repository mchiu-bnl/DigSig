
void v1742_toroot()
{
  gSystem->Load("libv1742prdf.so");
  pfileopen("data/junk_00000023-0000.prdf");
  prun(5000);

  cleanup();
}

