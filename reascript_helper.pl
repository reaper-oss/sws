#!/usr/bin/perl

# Original script by Cockos (http://landoleet.org/dev/sws/)
# JFB mods: tailored for the SWS/S&M extension: process ./ReaScript.cpp rather than ../plugin.cpp

sub ReadAPIFuncs
{
  @funclist=();
  %rtype={};
  %ptypes={};
  %pnames={};
  %callfuncs={};

  open INFILE, "ReaScript.cpp" or die "Can't read ReaScript.cpp";
  while (<INFILE>)
  {  
    if (/\s*\{\s*APIFUNC/)
    {          
      $fname="";   
      $callfunc="";
      if (/APIFUNC\((.+?)\),\s*(.*)/)
      {
        $callfunc=$fname=$1;
        $_=$2;
      } 
      elsif (/APIFUNC_NAMED\((.+?)\,(.+?)\),\s*(.*)/)
      {
        $fname=$1;
        $callfunc=$2;
        $_=$3;
      }    
      $fname or die "Can't parse function name";
      push @funclist, $fname;
      $callfuncs{$fname}=$callfunc;

      (/\"(.+?)\"\s*\,\s*(.*)/) or die "Can't parse return type";
      $rtype{$fname}=$1;
      $_=$2;
     
      (/\"(.*?)\"\s*\,\s*(.*)/) or die "Can't parse parameter types";
      $ptypes{$fname}=$1;
      $_=$2;
     
      (/\"(.*?)\"/) or die "Can't parse parameter names";
	  $pnames{$fname}=$1;	
    }
  }
  close INFILE;

  @funclist=sort {lc $a cmp lc $b} @funclist;
  return (\@funclist, \%rtype, \%ptypes, \%pnames, \%callfuncs); 
}

sub IsInt
{
  my %types=map { $_ => 1 } ( "int", "unsigned int", "UINT", "DWORD", "size_t", "LICE_pixel" );
  my ($t)=@_;
  return exists $types{$t};
}

sub  IsPlainData
{
  my %types=map { $_ => 1 } ( "bool", "char", "float", "double" );
  my ($t)=@_;
  return IsInt($t) || exists $types{$t};
}

sub IsPlainDataPtr
{
  my ($t)=@_;
  $t =~ s/\*// or return 0;
  return IsPlainData($t);
}

sub IsString
{
  my ($t)= @_;
  return ($t eq "char*" || $t eq "const char*");
}

sub IsOpaquePtr
{
  my ($t)=@_;
  $t eq "HWND" and return 1;
  $t =~ s/\*// or return 0;
  return (!IsPlainData($t) && $t ne "const char");
}

sub UnderlyingType
{
  my ($t)=@_;
  chop $t;
  return $t;
}



1;
