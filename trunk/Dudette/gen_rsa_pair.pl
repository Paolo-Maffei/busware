#!/usr/bin/perl
#
use strict;

my $res = qx!openssl genrsa -3 128 | openssl rsa -text!;

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
  while (@M>16) { shift @M; }
  $_ = '0x'.$_ foreach (@M);
#  $_ = hex($_) foreach (@M);
  printf STDERR "public: %s\n", join ' ', @M;
  printf join ' ', @M;
}
if ($L =~ /privateExponent:([0-9a-f\:]+)/ ) {
  my @M = split( /:/, $1);
  while (@M>16) { shift @M; }
  $_ = '0x'.$_ foreach (@M);
#  $_ = hex($_) foreach (@M);
  printf STDERR "private: %s\n", join ' ', @M;
  printf " %s\n", join ' ', @M;
}
