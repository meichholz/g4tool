#!/usr/bin/perl

while (<STDIN>)
 {
  chop;
  tr/a-z/A-Z/;
  s/\s//g;
  $sLine=$_;
  if (/[^A-Z0-9]/)
   {
    print "Fehler in Eingabe: $sLine\n";
    next;
   }
  for ($n=1; $n<=1; $n++)
   {
    for ($i=0; $i<length($sLine); $i++)
     {
      $ch=substr($sLine,$i,1);
      print $ch."    ";
     }
    print "\n";
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
    print "\n";
   }
 }