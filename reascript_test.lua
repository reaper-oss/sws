reaper.ShowConsoleMsg("");
ts,tn = reaper.GetProjectTimeSignature()

tr=reaper.GetTrack(0,0);
reaper.SNM_AddTCPFXParm(tr,0,0);
reaper.SNM_AddTCPFXParm(tr,0,1);
reaper.SNM_AddTCPFXParm(tr,0,2);

-- EEL: bool reaper.SNM_SetProjectMarker", ReaProject proj, int num, bool isrgn, pos, rgnend, "name", int color)
-- Lua: boolean reaper.SNM_SetProjectMarker(ReaProject proj, integer num, boolean isrgn, number pos, number rgnend, string name, integer color)
reaper.SNM_SetProjectMarker(0.0, 1.0, 1.0, 0.0, 2.0, "test NULL proj", 0);


-- EEL: bool reaper.SNM_GetFastString", #retval, WDL_FastString str)
-- Lua: string reaper.SNM_GetFastString(WDL_FastString str)
reaper.ShowConsoleMsg("SNM_CreateFastString+SNM_GetFastString: ");
w = reaper.SNM_CreateFastString("ok\n");
aa = reaper.SNM_GetFastString(w);
reaper.ShowConsoleMsg(aa);


--   { APIFUNC(SNM_test1), "int", "int*", "aOut", "", },
-- EEL: int reaper.SNM_test1", int &aOut)
-- Lua: integer retval, number aOut reaper.SNM_test1()
reaper.ShowConsoleMsg("SNM_test1\n");
test1r=0;
test1p=12;
test1r,test1p = reaper.SNM_test1();
if test1r == 666 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if test1p == 2 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end


--  { APIFUNC(SNM_test2), "double", "double*", "aOut", "", },
-- EEL: double reaper.SNM_test2", &aOut)
-- Lua: number retval, number aOut reaper.SNM_test2()
reaper.ShowConsoleMsg("SNM_test2\n");
test2r=0.0;
test2p=0.0;
test2r,test2p = reaper.SNM_test2();
if test2r == 666.777 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if test2p == 1.23456789 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end


--   { APIFUNC(SNM_test3), "bool", "int*,double*,bool*", "aOut,bOut,cOut", "", },
-- EEL: bool reaper.SNM_test3", int &aOut, &bOut, bool &cOut)
-- Lua: boolean retval, number aOut, number bOut, boolean cOut reaper.SNM_test3()
reaper.ShowConsoleMsg("SNM_test3 -- with all params, unlike doc\n");
test3=0; testa=333; testb=333.444; testc=false;
test3,testa,testb,testc = reaper.SNM_test3(testa,testb,testc);
if test3 == true then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if testa == 666 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if testb == 666.888 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if testc == true then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end

reaper.ShowConsoleMsg("SNM_test3 -- with no param, like doc\n");
test3=0; testa=333; testb=333.444; testc=false;
test3,testa,testb,testc = reaper.SNM_test3();
if test3 == true then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if testa == 0 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if testb == 0.0 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if testc == true then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end


--  { APIFUNC(SNM_test4), "void", "double", "a", "", },
-- EEL: reaper.SNM_test4", a)
-- Lua: reaper.SNM_test4(number a)
reaper.ShowConsoleMsg("SNM_test4\n");
test4=123.456789;
reaper.SNM_test4(test4);


--  { APIFUNC(SNM_test5), "void", "const char*", "a", "", },
-- EEL: reaper.SNM_test5", "a")
-- Lua: reaper.SNM_test5(string a)
reaper.ShowConsoleMsg("SNM_test5\n");
test5="SNM_test5";
reaper.SNM_test5(test5);


--  { APIFUNC(SNM_test6), "const char*", "char*", "a", "", }, -- not an "Out" parm
-- EEL: bool reaper.SNM_test6", #retval, #a)
-- Lua: string retval, string a SNM_test6(string a)
reaper.ShowConsoleMsg("SNM_test6\n");
test6p="SNM_test6";
test6r="SNM_test6";
test6r,test6p = reaper.SNM_test6(test6p);
reaper.ShowConsoleMsg(test6p);
if test6p == "SNM_test6 new parm val" then
  reaper.ShowConsoleMsg(": ok\n");
else
  reaper.ShowConsoleMsg(": KO !\n");
end
reaper.ShowConsoleMsg(test6r);
if test6r == "SNM_test6 return val" then
  reaper.ShowConsoleMsg(": ok\n");
else
  reaper.ShowConsoleMsg(": KO !\n");
end

-- == test3 somehow but opt params + return double on top
--  { APIFUNC(SNM_test7), "double", "int,int*,double*,bool*", "i,aOut,bOutOptional,cOutOptional", "", },
-- EEL: double reaper.SNM_test7", int i, int &aOut, optional &bOutOptional, optional bool &cOutOptional)
-- Lua: number retval, number aOut, optional number bOutOptional, optional boolean cOutOptional SNM_test7(integer i)
reaper.ShowConsoleMsg("SNM_test7 with all params (more than doc)\n");
test7=0.0; test7a=333; test7b=333.444; test7c=true;
ble="lkjlkj";
bla="cs";
test7,test7a,test7b,test7c,ble,bla = reaper.SNM_test7(7,test7a,test7b,test7c,ble,bla);
if test7 == 666.777 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if test7a == 666 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if test7b == 666.888 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if test7c == false then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if ble == "SNM_test7 new parm val" then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end

reaper.ShowConsoleMsg("SNM_test7 with min params (like the doc)\n");
test72p=12;
test72r,test72p = reaper.SNM_test7(7,test72p);
if test72r == 666.777 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if test72p == 24 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
-- and again...
test72r,test72p = reaper.SNM_test7(7);
if test72p == 0 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end


-- EEL: bool extension_api("SNM_test8", #retval, #buf1, "buf2", int i, #buf3, optional int &iOutOptional)
-- Lua: string retval, string buf1, string buf3, optional number iOutOptional reaper.SNM_test8(string buf1, string buf2, integer i, string buf3)
reaper.ShowConsoleMsg("SNM_test8\n");
buf1="buf11";
buf2="buf22";
buf3="buf33";
test8p=321;
test8r,buf1,buf3,test8p = reaper.SNM_test8(buf1,buf2,666,buf3,test8p);
if test8r == "SNM_test8" then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if buf1 == "SNM_test8 new parm val for buf1" then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if buf2 == "buf22" then -- this test is a bit silly (buf2 is const char*)
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if buf3 == "SNM_test8 new parm val for buf3" then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
if test8p==123456789 then
  reaper.ShowConsoleMsg("ok\n");
else
  reaper.ShowConsoleMsg("KO !\n");
end
