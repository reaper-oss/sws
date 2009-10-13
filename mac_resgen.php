#!/usr/bin/php
<?
if (!($fp = popen("find . -name \*.rc","r"))) die("Error searching\n");
$x = fread($fp,65536*16);
fclose($fp);

$pt=explode("\n",$x);

for ($x = 0; $x < count($pt); $x ++)
{
   $srcfn = $pt[$x];
   if (!stristr($srcfn,".rc")) continue;
   echo "processing " . $srcfn . "\n";
   $ofnmenu = $srcfn . "_mac_menu";
   $ofndlg = $srcfn . "_mac_dlg";
   system("php ../WDL/swell/rc2cpp_menu.php < \"$srcfn\" > \"$ofnmenu\"");
   system("php ../WDL/swell/rc2cpp_dlg.php < \"$srcfn\" > \"$ofndlg\"");
}

?>
