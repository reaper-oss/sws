# Original script by Cockos (http://landoleet.org/dev/sws/), also see http://code.google.com/p/sws-extension/issues/detail?id=432
# JFB mods: tailored for the SWS/S&M extension, i.e. simplified (use a reaper_python import), gets function pointers with rpr_getfp(), removed "RPR_" prefix

#!/usr/bin/perl

require "reascript_helper.pl";

$arg=shift @ARGV;
$arg eq "-x64" and $isx64=1;


($funclist, $rtype, $ptypes)=ReadAPIFuncs();


print <<EOF;
from reaper_python import *

EOF

sub PyType
{
  my ($t)=@_;
  $t eq "void" and return "None";
  ($t eq "bool" || $t eq "char") and return "c_byte";
  IsInt($t) and return "c_int";
  $t eq "float" and return "c_float";
  $t eq "double" and return "c_double";
  IsString($t) and return "c_char_p";
  IsPlainDataPtr($t) and return "c_void_p";
  IsOpaquePtr($t) and return ($isx64 ? "c_uint64" : "c_uint"); # includes void*
  die "unknown type $t";
}

sub PyPack
{
  my ($t, $v)=@_;
  IsPlainData($t) and return sprintf "%s(%s)", PyType($t), $v; 
  $t eq "const char*" and return sprintf "rpr_packsc(%s)", $v;
  $t eq "char*" and return sprintf "rpr_packs(%s)", $v;
  # todo handle char**
  IsPlainDataPtr($t) and return sprintf "%s(%s)", PyType(UnderlyingType($t)), $v;
  IsOpaquePtr($t) and return sprintf "rpr_packp('%s',%s)", $t, $v;	
  die "unknown type $t";
}

sub PyUnpack
{
  my ($t, $ov, $v)=@_;
  (IsPlainData($t) || IsOpaquePtr($t) || $t eq "const char*") and return $ov;
  $t eq "char*" and return sprintf "rpr_unpacks(%s)", $v;
  ($t eq "bool*" || $t eq "char*" || IsInt(UnderlyingType($t))) and
    return sprintf "int(%s.value)", $v;
  ($t eq "float*" || $t eq "double*") and return sprintf "float(%s.value)", $v;
  # todo handle char**
  IsPlainDataPtr($t) and return PyUnpack(UnderlyingType($t), $v);
  die "unknown type $t";
}

sub PyRetUnpack
{
  my ($t, $v)=@_;
  $t eq "const char*" and return sprintf "str(%s.decode())", $v;
  IsOpaquePtr($t) and return sprintf "rpr_unpackp('%s',%s)", $t, $v;
  return $v;
}


ReadAPIFuncs();

foreach $fname (@$funclist)
{
  $fname eq "ValidatePtr" and next;

  @inparms=();
  @proto=();
  @pack=();
  @cparms=();
  @unpack=();

  push @proto, PyType($rtype->{$fname});

  $byref=0;
  $i=0;
  foreach $t (split(",", $ptypes->{$fname}))
  { 
    $isptr=IsPlainDataPtr($t);

    push @inparms, "p$i";
    push @proto, PyType($t);
    push @pack, PyPack($t, "p$i");
	push @cparms, (($isptr && $t ne "char*") ? "byref(t[$i])" : "t[$i]");
    push @unpack, PyUnpack($t, "p$i", "t[$i]");
    $isptr and $byref=1;
    ++$i;
  }

  $isvoid=($rtype->{$fname} eq "void");

  printf "def $fname(%s):\n", join(",", @inparms);
  printf "  a=rpr_getfp('$fname')\n"; # errors out if key does not exist
  printf "  f=CFUNCTYPE(%s)(a)\n", join(",", @proto);
  @pack and printf "  t=(%s%s)\n", join(",", @pack), (@pack == 1 ? ",": "");
  printf "  %sf(%s)\n", ($isvoid ? "" : "r="), join(",", @cparms);
  if ($byref)
  {
    $isvoid or unshift @unpack, PyRetUnpack($rtype->{$fname}, "r");
	printf "  return (%s)\n", join(",", @unpack);
  }
  elsif (!$isvoid)
  {
    printf "  return %s\n", PyRetUnpack($rtype->{$fname}, "r");
  }
  print "\n";
}


warn "Parsed " . @$funclist . " functions\n";

