
TString get_runstr(const char *fname)
{
  TString name = fname;
  name.ReplaceAll(".prdf","");
  name.ReplaceAll(".root","");
  int index = name.Last('/');
  if ( index > 0 )
  {
    name.Remove(0,index+1);
  }
  index = name.First('-');
  if ( index > 0 )
  {
    name.Remove(0,index+1);
  }
  //cout << name << endl;

  index = name.Last('_');
  if ( index > 0 )
  {
    name.Remove(index,name.Length());
  }
  //cout << name << endl;

  return name;

}

