#!/usr/bin/perl
#
use strict;

my $bits  = shift @ARGV || 128;
my $bytes = $bits/8;

my $res = qx!openssl genrsa -3 $bits | openssl rsa -text!;

my $L = '';

foreach my $line (split( /\n/, $res)) {
  chomp( $line );
  if ($line =~ /^\s+/) {
    $line =~ s/^\s+//;
  } else {
    $L .= ' ';
  }
  $L .= $line;
} 

#printf "%s\n", "$L";

if ($L =~ /modulus:([0-9a-f\:]+)/ ) {
  my @M = split( /:/, $1);
  while (@M>$bytes) { shift @M; }
  $_ = '0x'.$_ foreach (@M);
#  $_ = hex($_) foreach (@M);
  printf STDERR "public: %s\n", join ',', @M;
  printf join ' ', @M;
}
if ($L =~ /privateExponent:([0-9a-f\:]+)/ ) {
  my @M = split( /:/, $1);
  while (@M>$bytes) { shift @M; }
  $_ = '0x'.$_ foreach (@M);
#  $_ = hex($_) foreach (@M);
  printf STDERR "private: %s\n", join ',', @M;
  printf " %s\n", join ' ', @M;
}
