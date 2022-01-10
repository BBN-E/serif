# Copyright 2008 by BBN Technologies Corp.
# All Rights Reserved.

############################################################################
# Copyright 2007 by BBN Technologies, LLC                                  #
#                                                                          #
# Use, duplication, or disclosure by the Government is subject to          #
# restrictions as set forth in subparagraph (c)(1)(ii) of the Rights in    #
# Technical Data and Computer Software clause at DFARS 252.227-7013.       #
#                                                                          #
#      BBN Technologies                                                    #
#      10 Moulton St.                                                      #
#      Cambridge, MA 02138                                                 #
#      617-873-3411                                                        #
#                                                                          #
############################################################################
####################################################################
# Sample script that sends a SHUTDOWN command to the Serif server. #
#                                                                  #
# $port - port on which to find the server.                        #
# $host - host machine for the server (defaults to local machine)  #
####################################################################

use strict;
use IO::Socket;

my $usage = "Usage: SerifShutdown.pl <port> [<host>]\n";
$usage = "$usage\tport - port on which to find the server\n";
$usage = "$usage\thost (optional) - host machine for the server (defaults to local machine)\n";

die $usage unless ((scalar (@ARGV) == 1) || (scalar (@ARGV) == 2));

my ($port, $host) = @ARGV;

$host = "localhost" unless $host;

#connect to the server
my $sock = new IO::Socket::INET (PeerAddr => $host,
				 PeerPort => $port,
				 Proto => 'tcp');

die "Socket creation failed: $!\n\n" unless $sock;

print $sock "8 SHUTDOWN";
my $line = <$sock>;
$line = <$sock>;
close $sock;
