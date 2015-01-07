<?

$fp= fopen("./ReaScript.cpp","r");
if (!$fp) die("error opening ReaScript.cpp");

date_default_timezone_set("America/New_York");
echo "// autogenerated by Jeffos' reascript_lua_eel.php -- " . strftime("%c") . "\n\n";

$funcs = array();
while (($x=fgets($fp,4096)))
{
    $aroffs=-1;
    if (preg_match('/\\s*{\\s*APIFUNC_NAMED\\(\\s*(.+?)\\s*,\\s*(.+?)\\s*\\)\\s*,\\s*"(.*?)"\\s*,\\s*"(.*?)"\\s*,\\s*"(.*?)".*$/',$x,$matches))
    {
      $aroffs=1;
    }
    else if (preg_match('/\\s*{\\s*APIFUNC\\(\\s*(.+?)\\s*\\)\\s*,\\s*"(.*?)"\\s*,\\s*"(.*?)"\\s*,\\s*"(.*?)".*$/',$x,$matches))
    {
      $aroffs=0;
    }

    if ($aroffs >= 0)
    {
        if (isset($funcs[$matches[1]]))
        {
          die("#error APIFUNC $matches[1] has duplicate entries\n");
        }

        if ($matches[1] == "time_precise")
        {
          continue;
        }
        
        
        $funcs[$matches[1]] = array("func" => $matches[1+$aroffs], 
                                    "ret" => $matches[2+$aroffs],
                                    "types" => $matches[3+$aroffs],
                                    "names" => $matches[4+$aroffs]);
    }
}
fclose($fp);

//ksort($funcs);


foreach ($funcs as $name => &$rec)
{
    $cfunc = "__eel_api_$name";

    $names = explode(",",$rec["names"]);
    $types = explode(",",$rec["types"]);
    if (count($names) != count($types))
    {
      die("#error $name has mismatched parmname/types\n");
    }

    $ret_type = $rec["ret"];
    $code = "";
    $code .= "static void* __luaeel_" . $name . "(void** arglist, int numparms)\n{\n  ";
    if ($ret_type != "void")
    {
      $code .= "return (void*)(INT_PTR)";
    }
    $code .= $name . "(";

    for ($x=0; $x < count($names); $x++)
    {
       $nm = trim($names[$x]);
       $type = trim($types[$x]);
       if ($type == "") continue;

       if ($type == "double" || $type == "int")
       {
         $code .= "(" . $type . ")(INT_PTR)arglist[" . $x . "]";
       }
       else
       {
         $code .= "(" . $type . ")arglist[" . $x . "]";
       }
       
       if ($x < count($names)-1)
       {
         $code .= ", ";
       }

       $parm_offs++;
       if ($next_sz) $x++;
    }

    $code .= ");\n";
    if ($ret_type == "void")
    {
      $code .= "  return NULL;\n";
    }
    $code .= "}\n\n";

    $rec["ret_type"] = $ret_type;
    $rec["cfunc"] = $cfunc;
    $rec["np"] = max($lastreq+1,1);
    $rec["npf"] = $parm_offs;

    echo $code;
}


?>