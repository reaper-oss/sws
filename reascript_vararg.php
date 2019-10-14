<?php

$fp= fopen("./ReaScript.cpp","r");
if (!$fp) die("error opening ReaScript.cpp");

echo "#ifndef _REASCRIPT_VARARG_H_\n";
echo "#define _REASCRIPT_VARARG_H_\n\n";

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
    $cfunc = "__vararg_$name";

    $names = explode(",",$rec["names"]);
    $types = explode(",",$rec["types"]);
    if (count($names) != count($types))
    {
      die("#error $name has mismatched parmname/types\n");
    }

    $ret_type = $rec["ret"];
    $code = "";
    $code .= "static void* " . $cfunc . "(void** arglist, int numparms)\n{\n  ";
    if ($ret_type != "void")
    {
      if ($ret_type == "double")
      {
        $code .= "double* p =(double*)arglist[numparms-1];\n";
        $code .= "  double d = ";
      }
      else
      {
        $code .= "return (void*)(INT_PTR)";
      }
    }
    $code .= $name . "(";

    $parm_offs = 0;
    $next_sz   = 0;
    $lastreq   = 0;
    for ($x=0; $x < count($names); $x++)
    {
       $nm = trim($names[$x]);
       $type = trim($types[$x]);
       if ($type == "") continue;

       if ($type == "int")
       {
         $code .= "(" . $type . ")(INT_PTR)arglist[" . $x . "]";
       }
       else if ($type == "double")
       {
         $code .= "arglist[" . $x . "] ? *(double*)arglist[" . $x . "] : 0.0";
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
    else if ($ret_type == "double")
    {
      $code .= "  if (p) *p=d;\n";
      $code .= "  return p;\n";
    }
    $code .= "}\n\n";

    $rec["ret_type"] = $ret_type;
    $rec["cfunc"] = $cfunc;
    $rec["np"] = max($lastreq+1,1);
    $rec["npf"] = $parm_offs;

    echo $code;
}

echo "\n#endif\n";

?>
