#!/usr/bin/perl
# nur ein kleines Hilfsprogramm, nun obsolet. �berspringt <arg> bytes.
$cch=shift;
read STDIN,$line,$cch;

read STDIN,$line,1e8;
print $line;
