#!/usr/bin/perl
#
################################################################
#
# filtg4
#
# Abnahme eines Multipage-G4-Files mit Jobparametern im Header
#
# Umwandlung in PCL mit Jobrahmen.
# !!! Gehört zu g4tool !!!
#
# (wird per make install nach /usr/stuff/lib/Watermark übertragen)
#
# (C) Marian Eichholz at DuSPat 1997
#
# v0.0 : 25.11.1997
# v0.9 : 28.11.1997 : Arbeitet normal, aber ohne Sortieren!
# v1.0 : 02.12.1997 : Kann auch Sortieren.
# v1.1 : 14.01.1998 : Loggt nach <SRC>/jobs, zentriert/clippt
# v1.2 : 19.01.1998 : A3-Support ("clip" und "format")
#
# Bis auf die PCL-Auflösung und die reine PCL-Graphik werden alle PJL und PCL-Jobparameter
# hier mit der Hand zusammengesetzt.
#
################################################################

# --------- Configuration
#
$sLogDirBase="/home/ME/src/unix/g4tool";
$sLogDir="$sLogDirBase/jobs";
$sSpoolBase="/var/spool/mopyscan";

	#
	# Böse Falle: Manueller Einzug ist Fach <2>.
	#             Fach 3 ist Umschlag!
	#
@aTrays=split(/,/,"0,2,1,4,5");

	#
	# Hier nochmal zur Klarheit:
	#
	# Geloggt wird ohne Rücksichtnahme auf Leserechte in das
        # .... g4tool/jobs
        #
        # Mit "bLogJob" wird der ganze Job geschrieben (eine Datei),
	# allerdings "aus dem Gedächtnis".
	#
	# MIt "bLogPages" dagegen werden die einzelnen Seiten mit
	# Auflösungshinweis geloggt.
	# Zweckmäßigerweise maht man die Änderung zum Abfangen kritischer
	# Seiten direkt im Installat (/usr/stuff/lib/Watermark)
	#

$bRealJob=1;	# 0 bringt direkten Pipe nach /tmp/g4job
$bVerbose=1;
$bLogPages=0;	# verhindert auch Drucken (Dummy-Jobframe wird geschrieben)
$bLogJob=0;	# Alle Jobdaten werden gleitend wieder ausgeschrieben

&Init;		# Initialisieren und ggd. abbrechen
&MainProgram;
exit 0;

#############################################################################

#
# Aufruflitanei laut Log:
# -P'mopyscan'
# -w'80' -l66 -x0 -y0
# -N'/var/samba/lpspool/KO.a29384'
# -S'G4-Decoder-Mopier'
# -Y0 -J'/var/samba/lpspool/KO.a29384 '
# -C'archie' -n'ko' -h'archie'
# -Ff'/usr/spool/mopyscan/acct'
#

###########################################################
# DumpSource: ggf. wird die Quell-Datei direkt in /tmp/g4job geschrieben.
###########################################################

sub DumpSource
 {
  $sLog="/tmp/g4job";
  open (LOG,">$sLog");
  while (read STDIN,$sBuffer,8192)
   {
    print LOG $sBuffer;
   }
  close(LOG);
  exit 0;
 }

###########################################################
# Init : Eröffnung des Programms
###########################################################

sub Init
 {
  if ($ARGV[0] eq 'HURGA')
   {
    $bRealJob=1;
    $sLogDir=$sSpoolBase=$sLogDirBase=".";
   }
  &DumpSource if !$bRealJob;
	#
	# Das Logging für den ganzen Job wird -wenn möglich - aktiviert
	#
  if ($bLogJob)
   {
    if (!open LOG,">$sLogDir/lpdjob.$$") { $bLogJob=0; }
   }

$sDefaults="format:A4,eco:OFF,copies:1,name:MopyScan,mopies:1,duplex:1,resolution:300,"
		."clip:0,sort:0,outbin:9,tray:4";
  &GetJobParm($sDefaults);
 }

###########################################################
# GetJobForm : Auswertung eines Job-Parameterstrings
###########################################################

sub GetJobParm	# in: parameterstring
 {
  local (@aParm,$sJobParm,$sKey,$sValue);
  $_=$_[0];
  @aParm=split(/,/);
  foreach $sJobParm(@aParm)
   {
    ($sKey,$sValue)=split(/:/,$sJobParm);
    $aJobParm{$sKey}=$sValue;
    print STDERR "got $sKey=$sValue\n" if $bVerbose;
   }
 }

###########################################################
# Ausgabe des Jobheaders
###########################################################

sub DoJobHead # nCopies, nOutbin
 {
  local($nTray,$nOutbin,$nCopies);
  print "\033%-12345X";
  print "\@PJL RDYMSG DISPLAY = \"$aJobParm{'name'}\"\n";
  print "\@PJL SET ECONOMODE = $aJobParm{'eco'}\n";
  print "\@PJL SET PAPER = $aJobParm{'format'}\n";
  $nCopies=$_[0];
  if ($aJobParm{'mopies'} == 0)
   { print "\@PJL SET COPIES=$nCopies\n"; }
  else
   { print "\@PJL SET QTY=$nCopies\n"; }
  print "\@PJL ENTER LANGUAGE=PCL\n";
  $nOutbin=$_[1];
  print STDERR "Copies:$nCopies, Outbin:$nOutbin\n" if $bVerbose;
  $nTray=$aTrays[$aJobParm{'tray'}];
  my $nPaper=26; # A4
  $nPaper=27 if $aJobParm{'format'} eq "A3";
  printf "\033E\033&l%dH\033&l%dG\033&l%dS\033&l%dA",
	$nTray,$nOutbin,$aJobParm{'duplex'},$nPaper;
 }

###########################################################
# Finish print job
###########################################################

sub DoJobTail
 {
  print "\033%-12345X\@PJL RDYMSG DISPLAY = \"\"\n\033%-12345X";
 }

###########################################################
# Sortiere Kopienjobs
###########################################################

sub SortCopies
 {
  local($nNextBin=0);	# interne Zählung 0...4
  	#
  	# Im Sortierfall muß die Mimik phasenverschoben durchgeführt werden.
  	# Jede Kopie wird ein eigener Job!
  	#
  for ($nCopy=1; $nCopy<=$aJobParm{'copies'}; $nCopy++)
   {
    print STDERR "unspooling copy #$nCopy...\n" if $bVerbose;
    &DoJobHead(1,$nNextBin+4);
    for ($iPage=1; $iPage<$iPageExt; $iPage++)
     {
      $sTmp=$aPageFileName[$iPage];
      print STDERR "unspooling $sTmp...\n" if $bVerbose;
      if (open (SINK,"<$sTmp"))
       {
        while (read SINK,$sPage,8192)
         { print $sPage; }
        close SINK;
       }
      else
       {
        print STDERR "sorting: cannot get pcl file $sTmp!\n";
       }
     }
    &DoJobTail;
    $nNextBin++;
    $nNextBin=0 if $nNextBin>=5;
   }
	#
	# Und am Ende werden alle PCL-Files wieder zerstört!
	#
  print STDERR "cleaning up...\n" if $bVerbose;
  for ($iPage=1; $iPage<$iPageExt; $iPage++)
   {
    unlink $aPageFileName[$iPage];
   }
 } # SortCopies
 
########################################################################
#                               MAIN                                   #
########################################################################

	#
	# Zum Joblogging:
	#
	# Alle relevante Information wird auch nach LOG geschrieben
	# (wurde oben geöffnet).
	#

sub MainProgram
 {

  #
  # Prüfung der Datei auf Echtheit
  #
  read STDIN,$sHead,4;
  if ($sHead ne '##G4')
   {
    print STDERR "Header not G4-Job, aborting...\n";
    exit 1;
   }
  $_=<STDIN>; # rest is junk
  print LOG "##G4".$_ if $bLogJob;

	#
	# Abnahme der Job-Parameter
	#
  $sLine=<STDIN>; chop $sLine;
  print LOG $sLine."\n" if $bLogJob;
  &GetJobParm($sLine);

  	#
  	# Abnahme der Seiten.
  	# Jeder Job beginnt mit einer Parameterzeile
  	#
  $bSort=$aJobParm{'sort'};
  $bPortrait=1;
  $bPortrait=1 if $aJobParm{'format'} eq "A3";
  &DoJobHead($aJobParm{'copies'},$aJobParm{'outbin'}) if !$bSort;
  $iPageExt=1;	# globaler(!) Zähler für externe Seiten
  while (<STDIN>)
   {
    print LOG if $bLogJob;	# Zeile loggen
    chop;
    #
    # Seitendaten abnehmen.
    #
    local (@aParm);
    @aParm=split(/,/);
    next if $#aParm<3;
    foreach $sPageParm (@aParm)
     {
      ($sKey,$sValue)=split(/:/,$sPageParm);
      $aPageParm{$sKey}=$sValue;
     }
    $nSize=$aPageParm{'size'};
    $cx=$aPageParm{'cx'};
    $cy=$aPageParm{'cy'};
    $iPage=$aPageParm{'page'};
    if ($nSize eq '')
     { print STDERR "cannot get size, aborting page\n"; next; }
    if ($nSize==0)
     { print STDERR "empty page, aborting...\n"; next; }
    #
    # lesen der Komplettseite in einen Puffer
    #
    print STDERR "Reading page (cx=$cx, cy=$cy, $nSize bytes)...\n" if $bVerbose;
    $nRead=read(STDIN,$sPage,$nSize);
    # print STDERR "got $nRead reported bytes.\n";
    	#
    	# Auf Wunsch wird die gesamte Seite undekodiert in eine Logdatei
    	# geschrieben ("1234.1.600x400.g4" z.B.).
    	#
    if ($bLogPages)
     {
      $sOName="$sLogDir/$$.$iPage.".$cx."x".$cy.".g4";
      print STDERR "Writing page log $sOName...\n" if $bVerbose;
      open OUT,">$sOName";
      print OUT $sPage;
      close (OUT);
     }
    else 
     {
     	#
     	# Dann wird mit dem g4tool die Seite in PCL dekodiert, und in eine
     	# Zwischendatei geschrieben. Diese kann von Sorter verwendet werden,
     	# oder wird nach Übertragung gelöscht.
     	#
      $nReso=$aJobParm{'resolution'};
      $sTmp="$sSpoolBase/filtg4.$$.$iPage";
      $aPageFileName[$iPageExt]=$sTmp;
      my $sOrient="l";
      $sOrient="p" if $bPortrait;
      $sClip="";
      $sClip="-M" if $aJobParm{'clip'}; # marginclip
      open SINK, "| /usr/stuff/bin/g4tool -P P$nReso -P $sOrient $sClip -o p -w $cx -h $cy -W l -W e >$sTmp" || die "fatal: g4tool doesnt come up";
      print SINK $sPage;
      print LOG  $sPage if $bLogJob;	# ggf. auch undekodiertes LOGging.
      close(SINK);
      	#
      	# Nun wird die Zwischendatei abschnittsweise auf die Ausgabe gelegt.
      	# Nur, wenn nicht sortiert wird, natürlich :-)
      	#
      if (!$bSort)
       {
        if (open (SINK,"<$sTmp"))
         {
          while (read SINK,$sPage,8192)
           { print $sPage; }
          close SINK;
          unlink $sTmp;
         }
        else
         {
          print STDERR "direct: cannot get pcl file $sTmp!\n";
         }
       }
     }
    $iPageExt++;
   }
  &DoJobTail if !$bSort;	# Job direkt abschließen
  &SortCopies if ($bSort);
  print STDERR "ready.\n" if $bVerbose;
 } # MainProgram

# ENDE filtg4

