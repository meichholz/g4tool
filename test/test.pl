#!/bin/perl -w

##################################################
# Test::Suite
#
# 1998, schilli@perlmeister.com
# 1999, eichholz@computer.org : Gepatcht und erweitert
#
#       1999 : Korrektur des Updatings. Output wird nach ".bogus" geschrieben.
# 19.11.1999 : Neuer Diff-Mode
##################################################

use File::Copy;
use File::Basename;
use Cwd;

package Test::Suite;

##################################################
sub new {                            # Constructor
##################################################
    my $class = shift;

    my $self  = {};
    bless($self, $class);
}

##################################################
sub register {                  # Register scripts
##################################################
    my ($self, $testname, @scripts) = @_;

    push(@{$self->{tests}}, [$testname, @scripts]);
}

##################################################
sub update {          # Update all reference files
##################################################
    my ($self) = @_;

    foreach $test (@{$self->{tests}}) {    
        my ($testname, @scripts) = @$test;

        foreach $script (@scripts) {    
            test_script($script, 1);
        }
    }
}

##################################################
sub clean {         # Clean up all reference files
##################################################
    my ($self) = @_;
    my $file;

    foreach $test (@{$self->{tests}}) {    
        my ($testname, @scripts) = @$test;

        foreach $script (@scripts) {    
            foreach $file ("$script.ref",
            		   "$script.ref.bogus",
                           "$script.ref.bak") {
                if(-e $file) {
                    print "Cleaning up $file\n";
                    unlink($file) ||  
                        die "Cannot unlink $file";
                }
            }
        }
    }
}

##################################################
sub diff {          # List differences in last run
#                   # by MME at DUSPAT 1999
##################################################
  my ($self,$bSideBySide) = @_;
  my $file;
  
  foreach $test (@{$self->{tests}}) {    
    my ($testname, @scripts) = @$test;
    
    foreach $script (@scripts) {    
      {
	my $sRef="$script.ref";
	my $sLast="$sRef.bogus";
	if (-e $sLast)
	  {
	    print "$script (should be <-- --> last run)\n";
	    my @aProg="diff";
	    push @aProg,"-y" if $bSideBySide;
	    push @aProg,$sRef,$sLast;
	    system @aProg;
	  } # if
      } # foreach script
    } # foreach test
  }
}

##################################################
sub run {
##################################################
    my $self = shift;

    foreach $test (@{$self->{tests}}) {    
        my ($testname, @scripts) = @$test;
        my $total = @scripts;
        my ($failed, $success) = (0,0);

        foreach $script (@scripts) {    
            if(test_script($script)) {
                $success++;
            } else {
                print "$testname: " .
                      "$script REF mismatch\n";
                $failed++;
            }
        }

        my $dots = "";
        if(length("$testname$total") < 40) {
            $dots = "." x 
             (40 - 2 - length("$testname$total"));
        }

        if($total eq $success) {
           print "$testname ($total) $dots ok\n";
	   return 0;
        } else { 
           print "$testname ($total) $dots " . 
                 "$success ok, $failed not ok\n";
	   return 1;
        }
    }
}

##################################################
sub test_script {
##################################################
    my ($script, $create_ref) = @_;
    my $shouldbe;
    my $retval = 0;

    my $cwd = Cwd::cwd();      # Get current dir

        # Change to dir where script resides in
    my($base, $path) = 
              File::Basename::fileparse($script);
    chdir($path) || die "Chdir $path failed";

    open(PIPE, "$base |") || 
        die "Cannot open $script";
    my $output = join('', <PIPE>);
    close(PIPE) || die "Script $script fails";

    if(! -f "$base.ref") {
        # REF file doesn't exist
        if($create_ref) {
            open(FILE, ">$base.ref") ||
                die "Cannot create $script.ref";
            print FILE $output;
            close(FILE);
            print "Created REF $script.ref\n";
        } else {
            die "No reference file for $script";
        }
    } else {
        # REF file does exist, read it
        open(FILE, "<$base.ref") ||
            die "Cannot open $script.ref";
        $shouldbe = join('', <FILE>);
        close(FILE);
	my $sLogFile="$base.ref.bogus";
        if($create_ref) {
            if($output ne $shouldbe) {

                # Create backup
                File::Copy::copy("$base.ref", 
                               "$base.ref.bak") ||
                    die "Backup $base.ref failed";

                # Write new reference file
                open(FILE, ">$base.ref") || 
                    die "Cannot write $script.ref";
                print FILE $output;
                close(FILE);
                print "Updated REF $script.ref\n";
            }
        } else {
        	# Der fehlerhafte Lauf wird auch protokolliert! (ME, 25.6.1999)
                open(FILE, ">$sLogFile") || 
                    die "Cannot write $script.ref.bogus";
                print FILE $output;
                close(FILE);
            $retval = ($output eq $shouldbe);
	    unlink $sLogFile if $retval;
        }
    }
    
    chdir($cwd) || die "Cannot chdir to $cwd";

    return($retval);
}

1;

# ################################################################
#
# Test::Driver
#
# Das Ding ist ein einfacher Testrahmen, der die *komplette* Ablaufsteuerung
# einer Testsuite beinhaltet.
#
# Dadurch können Verbesserungen leichter in vielen Testsuiten benutzt werden.
#
# Ein vollständiger Client kann so aussehen:
#BEGIN { push @INC,"/usr/stuff/lib/perl"; }
#use Test::Driver;
#my @aSuite=([ "Test 1", "Filter/access.sh", "Filter/volltext.sh"],
#            [ "Test 2", "Interna/test.sh" ]);
#my $driver=Test::Driver->new(\@aSuite);
#exit $driver->Run("dotests.pl",\@ARGV);
# #################################################################

package Test::Driver;

# use Test::Suite;

sub new {
  my ($class,$raSuite)=@_;
  my $self={};
  $self->{SPEC}=$raSuite;
  bless $self,$class;
}

sub Run {
  my ($self,$sName,$raArg)=@_;
  my $raSuite=$self->{SPEC};
  if ($#{$raArg}<0)
  {
    my $i;
    print "usage:\t$sName [-a] [-create|-clean|-diff|-ydiff] {<test>}\n";
    print "test:\t";
    for ($i=0; $i<=$#{$raSuite}; $i++)
      {
	print "\n\t" if $i;
	print $i+1," : ",$raSuite->[$i]->[0];
      }
    print "\n";
    return 1;
  }

  my $suite=new Test::Suite;
  my %hTypes;
  map { $hTypes{$1}=1 if /^(\d+)$/; } @$raArg;

  my $i;
  for ($i=1; $i<=$#{$raSuite}+1; $i++)
    {
      $suite->register(@{$raSuite->[$i-1]}) if (!%hTypes) or $hTypes{$i};
    }
  
  if (grep {$_ eq "-create"} @$raArg) { $suite->update(); }
  elsif (grep { $_ eq "-clean"} @$raArg) { $suite->clean(); }
  elsif (grep { $_ eq "-diff"} @$raArg) { $suite->diff(0); }
  elsif (grep { $_ eq "-ydiff"} @$raArg) { $suite->diff(1); }
  else { return $suite->run(); }
  return 0;
}

1;

# ######################################################################

package main;

# use Test::Driver;

my $suite=Test::Driver->new([
	[ "g4tool", "g4tool/basic.sh" ]
	]);
exit $suite->Run("test.pl",\@ARGV);

