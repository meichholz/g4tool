#!/usr/bin/perl
#
###############################
#
# dumpbits
#
# schreibt Hexdump/ Bitdump für Binärdateien
#
###############################

$iFirst=0;
$iLast=512;

for ($i=0; !eof(STDIN) && $i<$iFirst; $i++)
 {
  getc(STDIN);
 }

$cBytes=$iLast-$iFirst;

while ($cBytes>0)
 {
  $sLine="";
  for ($i=0; $i<8; $i++)
   {
    if (eof(STDIN))
     {
      $sLine=$sLine."00";
     }
    else
     {
      # read(STDIN,$ch,1);
      $sLine=$sLine.sprintf "%02X",ord(getc(STDIN));
     }
   }
  $cBytes=$cBytes-8;
  printf "%06X:",$iFirst;
  for ($i=0; $i<length($sLine); $i++)
   {
    $ch=substr($sLine,$i,1);
    print $ch."    ";
   }
  print "\n       ";
  for ($i=0; $i<length($sLine); $i++)
   {
    $ch=substr($sLine,$i,1);
    $nValue=index("0123456789ABCDEF",$ch);
    for ($b=3; $b>=0; $b--)
     {
      if ($nValue&(1<<$b))
       {
        print "1";
       }
      else
       {
        print "0";
       }
     }
    if ($i&1) { print " " } else { print "."; }
   }
  $iFirst=$iFirst+8;
  print "\n\n";
 }