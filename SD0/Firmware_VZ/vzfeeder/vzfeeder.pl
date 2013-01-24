#!/usr/bin/perl

use warnings;
use strict;
use Time::HiRes qw/ gettimeofday /;;
use POE;
use POE::Wheel::ReadWrite;
use POE::Wheel::ReadLine;
use POE::Component::Client::HTTP;
use POE::Component::DBIAgent;
use Symbol qw(gensym);
use Device::SerialPort;
use POE::Filter::Line;
use HTTP::Request;
use Data::Dumper;
use DateTime;

sub DB_NAME () { "volkszaehler" }
sub DB_USER () { "vz" }
sub DB_PASS () { "demo" }

use Log::Log4perl qw/:easy/;
Log::Log4perl->easy_init( {
                           level  => $INFO,
                           layout => '[%d] %p %c - %m%n',
                          } );

INFO('starting logger process');

POE::Session->create(
  inline_states => {
    _start      => \&setup_device,
    open_serial => \&open_serial,
    program_serial => \&program_serial,
    got_serial  => \&got_serial,
    got_error   => \&handle_errors,
    cmd_V       => \&got_version,
    cmd_L       => \&got_portbits,
    queue_cmd   => \&queue_cmd,

    mod_reset   => \&mod_reset,
    mod_bl      => \&mod_bl,

    query_all_channels_done => \&handle_query_all_channels_response,
    finish_query_all_channels => \&finish_query_all_channels,
  },
)->option( trace => 1 );

POE::Kernel->run();
exit 0;

sub setup_device {
  my ($kernel, $heap, $session) = @_[KERNEL, HEAP, SESSION];

  POE::Component::Client::HTTP->spawn(
                                      Alias     => 'ua',
                                      Timeout   => 10,
                                     );

  INFO('opening database link');
  my $dbi = $heap->{dbi_helper} = POE::Component::DBIAgent->new(
    DSN     => ['dbi:mysql:dbname=' . DB_NAME, DB_USER, DB_PASS],
    Count   => 3,
    Queries => {
      query_all_channels  => 'select * from entities',
      insert_tickdata     => 'insert into data (timestamp,channel_id,value) values (?, ?, ?)',
#      update => "update test set value = ? where name = ?",
#      delete => "delete from test where name = ?",
    },
  );

  $heap->{CHANNELS} = [];
  $dbi->query(query_all_channels => $session->ID => 'query_all_channels_done');

  $kernel->yield( 'open_serial' );
}

# open serial Port
sub open_serial {
  my ($kernel, $heap, $session, $data) = @_[KERNEL, HEAP, SESSION, ARG0];

  INFO('opening serial port');

  qx^if test ! -d /sys/class/gpio/gpio27; then echo 27 > /sys/class/gpio/export; fi^;
  qx^echo out > /sys/class/gpio/gpio27/direction^;
  qx^echo 1 > /sys/class/gpio/gpio27/value^;
  qx^if test ! -d /sys/class/gpio/gpio17; then echo 17 > /sys/class/gpio/export; fi^;
  qx^echo out > /sys/class/gpio/gpio17/direction^;
  qx^echo 0 > /sys/class/gpio/gpio17/value^;
  qx^echo 1 > /sys/class/gpio/gpio17/value^;

  # Open a serial port, and tie it to a file handle for POE.
  my $handle = gensym();
  my $port = tie(*$handle, "Device::SerialPort", "/dev/ttyAMA0");
  die "can't open port: $!" unless $port;

  $port->datatype('raw');
  $port->reset_error();
  $port->baudrate(38400);
  $port->databits(8);
  $port->parity('none');
  $port->stopbits(1);
  $port->handshake('none');

  $port->write_settings();

  $port->lookclear();

  # Start interacting with the GPS.
  $heap->{port}       = $port;
  $heap->{port_wheel} = POE::Wheel::ReadWrite->new(
    Handle => $handle,
    Filter => POE::Filter::Line->new(
      InputLiteral  => "\x0D\x0A",    # Received line endings.
      OutputLiteral => "\x0D",        # Sent line endings.
    ),
    InputEvent => "got_serial",
    ErrorEvent => "got_error",
  );

  $kernel->delay_add( queue_cmd => 2 => 'v' ); 
  $kernel->delay( program_serial => 5 ); 
}

# programming via serial Port
sub program_serial {
  my ($kernel, $heap, $session, $data) = @_[KERNEL, HEAP, SESSION, ARG0];

  INFO('flashing module @ serial port');
  
  delete $heap->{port_wheel};
  untie( $heap->{port} ) if $heap->{port};
  delete ( $heap->{port} );

  qx^if test ! -d /sys/class/gpio/gpio17; then echo 17 > /sys/class/gpio/export; fi^;
  qx^echo out > /sys/class/gpio/gpio17/direction^;
  qx^echo 0 > /sys/class/gpio/gpio17/value^;
  qx^if test ! -d /sys/class/gpio/gpio27; then echo 27 > /sys/class/gpio/export; fi^;
  qx^echo out > /sys/class/gpio/gpio27/direction^;
  qx^echo 0 > /sys/class/gpio/gpio27/value^;
  qx^echo 1 > /sys/class/gpio/gpio17/value^;
  qx^sleep 1^;
  qx^echo 1 > /sys/class/gpio/gpio27/value^;
  qx^avrdude -p atmega1284p -P /dev/ttyAMA0 -b 38400 -c avr109 -U flash:w:main.hex^;

  $kernel->yield( 'open_serial' );
}


# Port data (lines, separated by CRLF) are displayed on the console.
sub got_serial {
  my ($kernel, $heap, $session, $data) = @_[KERNEL, HEAP, SESSION, ARG0];
  INFO( 'got_serial: ' . $data );
  my @v = split( /\s+/, $data );

  # Channel ticks?
  if ($v[0] =~ /^([ABCD])$/) {
    my $ch = $1;
    INFO( 'Tick @ ' . $ch );
    my ($seconds, $microseconds) = gettimeofday;
    my $sec = sprintf( '%d%03d', $seconds, $microseconds/1000 );

    $heap->{dbi_helper}->query( insert_tickdata => $session->ID => undef => ($sec, ord($ch)-64, 1));

    return;
  } 

  my $cmd = sprintf 'cmd_%s', shift @v;
  $kernel->yield( $cmd => [@v] );
}

# Error on the serial port.  Shut down.
sub handle_errors {
  my $heap = $_[HEAP];
  ERROR( 'received error!' );
  delete $heap->{port_wheel};
}

sub got_version {
  my ($kernel, $heap, $data) = @_[KERNEL, HEAP, ARG0];
  INFO( 'VERSION: ' . $data->[0] );
  $heap->{VERSION} = $data->[0];

  $kernel->delay( 'program_serial' );
}

sub got_portbits {
  my ($kernel, $heap, $data) = @_[KERNEL, HEAP, ARG0];
  INFO( 'Portbits: ' . $data->[0] );
  $heap->{PORTBITS} = $data->[0];
}

sub queue_cmd {
  my ($kernel, $heap, $data) = @_[KERNEL, HEAP, ARG0];
  $heap->{port_wheel}->put( $data );
}

#
# DATABASE work
#

sub handle_query_all_channels_response {
  my ($kernel, $heap, $data) = @_[KERNEL, HEAP, ARG0];

  if (ref($data) eq 'ARRAY') {
    push $heap->{CHANNELS}, $data;
  } elsif ($data eq 'EOF') {
    $kernel->yield( finish_query_all_channels => $heap->{CHANNELS} );
  }

}

sub finish_query_all_channels {
  my ($kernel, $heap, $data) = @_[KERNEL, HEAP, ARG0];
  INFO( Dumper($heap->{CHANNELS}));
}

