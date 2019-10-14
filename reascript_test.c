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
  sprintf(buf, "-------------> a = %d\r\n", a?*a:-1);
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
const char* SNM_test8(char* buf1, int buf1_sz, const char* buf2, int buf2_sz, int i, char* buf3, int buf3_sz, int* iOptionalOut)
{
  char buf[128]="";
  sprintf(buf, "---> %s\r\n", buf1);
  ShowConsoleMsg(buf);
  sprintf(buf, "---> %s\r\n", buf2);
  ShowConsoleMsg(buf);
  sprintf(buf, "---> %s\r\n", buf3);
  ShowConsoleMsg(buf);
  sprintf(buf, "---> %d\r\n", i);
  ShowConsoleMsg(buf);
  if (iOptionalOut) { sprintf(buf, "---> *iOptionalOut = %d\r\n", *iOptionalOut); *iOptionalOut=123456789; }
  else strcpy(buf, "---> null iOptionalOut\r\n");
  ShowConsoleMsg(buf);
  if (buf1) lstrcpyn(buf1,"SNM_test8 new parm val for buf1",buf1_sz);
  if (buf3) lstrcpyn(buf3,"SNM_test8 new parm val for buf3",buf3_sz);
  return "SNM_test8";
}
