int SNM_test1(int* p)
{
  if (p) *p = 2;
  return 666;
}
double SNM_test2(double* p)
{
  if (p) *p=1.23456789;
  return 666.777;
}
bool SNM_test3(int* a, double* b, bool* c)
{
  if (a) *a *= 2;
  if (b) *b *= 2.0;
  if (c) *c= !(*c);
  return true;
}
void SNM_test4(double p)
{
  char buf[128]="";
  sprintf(buf, "---> %f\r\n", p);
  ShowConsoleMsg(buf);
}
void SNM_test5(const char* s)
{
  char buf[128]="";
  sprintf(buf, "---> %s\r\n", s?s:"null prm");
  ShowConsoleMsg(buf);
}
const char* SNM_test6(char* s)
{
  char buf[128]="";
  sprintf(buf, "---> %s\r\n", s);
  ShowConsoleMsg(buf);
  if (s) strcpy(s,"SNM_test6 new parm val");
  return "SNM_test6 return val";
}
double SNM_test7(int i, int* a, double* b, bool* c, char* s, const char* cs)
{
  char buf[128]="";
  sprintf(buf, "---> %d\r\n", i);
  ShowConsoleMsg(buf);
  if (a) *a *= 2;
  if (b) *b *= 2.0;
  if (c) *c= !(*c);
  sprintf(buf, "---> %s\r\n", s?s:"null prm");
  ShowConsoleMsg(buf);
  if (s) strcpy(s,"SNM_test7 new parm val");
  sprintf(buf, "---> %s\r\n", cs?cs:"null prm");
  ShowConsoleMsg(buf);
  return 666.777;
}
