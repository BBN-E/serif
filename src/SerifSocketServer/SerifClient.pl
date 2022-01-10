# Copyright 2008 by BBN Technologies Corp.
# All Rights Reserved.

############################################################################
# Copyright 2008 by BBN Technologies, LLC                                  #
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
#######################################################################
# Sample client script for Serif. This script sends a document to the #
# SerifServer, retrieves the results, and prints it out to a results  #
# file.                                                               #
# $infile - file containing the document to be processed              #
# $outfile - file where the results are to be written                 #
# $port - port on which to find the server                            #
# $host - host machine for the server (defaults to local machine)     #
#######################################################################

use strict;
use IO::Socket;
use File::Basename;

my $usage = "Usage: SerifClient.pl <infile> <outfile> <port> [-host <host>] [-text <text>] [-docid <docid>] [-ignore] <ignore>]\n";
$usage = "$usage\tinfile - file containing the document to be processed\n";
$usage = "$usage\toutfile - file where the results are to be written\n";
$usage = "$usage\tport - port on which to find the server\n";
$usage = "$usage\t-host <host> (optional) - host machine for the server (defaults to local machine)\n";
$usage = "$usage\t-text <text> (optional) - tag to denote text to be processed\n";
$usage = "$usage\t-docid <docid> (optional) - tag to denote the document number\n";
$usage = "$usage\t-ignore <ignore> (optional) - tag to denote text portions not to be processed\n";


my $arg_count = scalar (@ARGV);

die $usage unless (($arg_count == 3) || ($arg_count == 5) ||
		   ($arg_count == 7) || ($arg_count == 9) ||
		   ($arg_count == 11));

my $infile_par = shift @ARGV;
my $outfile_par = shift @ARGV;
my $port = shift @ARGV;

my ($flag, $arg, $host, $text_tag, $docno_tag, $ignore_tag);

$infile_par =~ s/\\/\//g;
if (-d $infile_par && !-d $outfile_par)
{
    die "If input parameter is a directory, then output parameter must be a directory as well.\n\n";
}

while (@ARGV)
{
    $flag = shift @ARGV;
    $arg = shift @ARGV;

    if ($flag eq "-host")
    {
	$host = $arg;
    }
    elsif ($flag eq "-text")
    {
	$text_tag = $arg;
    }
    elsif ($flag eq "-ignore")
    {
	$ignore_tag = $arg;
    }
    elsif ($flag eq "-docno")
    {
	$docno_tag = $arg;
    }
    else
    {
	die $usage;
    }
}

$host = "localhost" unless $host;

my ($sock, $status, $response, $document, $apf);
my $OK = 1;
my $ERROR = 0;

my @files = ();
if (-d $infile_par) {
    @files = glob( "$infile_par/*" );
    print "Working on " . scalar (@files) . " files.\n";
} else {
    @files = ( $infile_par );
}

foreach my $infile ( @files ) {
    #read in the document
    print "Reading in document...";
    ($document, $status) = &fileToString ($infile);
    die "Problem reading $infile: $status\n\n" unless ($status eq "OK");
    
    if ($text_tag)
    {
	$document =~ s/\<$text_tag\>/\<TEXT\>/gs;
	$document =~ s/\<\/$text_tag\>/\<\/TEXT\>/gs;
    }
    if ($docno_tag)
    {
	$document =~ s/\<$docno_tag\>/\<DOCID\>/gs;
	$document =~ s/\<\/$docno_tag\>/\<\/DOCID\>/gs;
    }
    if ($ignore_tag)
    {
	$document =~ s/\<$ignore_tag\>/\<IGNORE\>/gs;
	$document =~ s/\<\/$ignore_tag\>/\<\/IGNORE\>/gs;
    }

    print "done\n";

    #connect to the server
    print "Connecting to Serif Server on $host on port $port...";
    $sock = new IO::Socket::INET (PeerAddr => $host,
				  PeerPort => $port,
				  Proto => 'tcp');
    
    die "Socket creation failed: $!\n\n" unless $sock;
    print "done\n";
     
    #make sure printing to the socket isn't buffered and delayed
    $sock->autoflush (1);
    
    #send the document to the server for Serif analysis
    print "Sending document to Serif Server for analysis...";
    my $start_time = time;
    $response = &sendCommand ($sock, "ANALYZE $document");
    print "done\n";
    print "Time: " . ( time - $start_time ) . " seconds\n";
    
    print "Writing results to file...";
    if (-d $outfile_par) {
	my $basename = basename($infile);
	open OUT, ">$outfile_par/$basename.apf" or die "Can't open $outfile_par/basename.apf\n\n";
    } else {
	open (OUT, ">$outfile_par") or die "Can't open $outfile_par\n\n";
    }

    print OUT $response;
    close OUT;
    print "done\n\n";

}
#########################################################################

#sendCommand sends a command message to the Serif Server. The server
#accepts commands via a socket connection, and returns a three part
#response ove rthe same socket. 
#
#The first element of the response is a status. A value of 1 signifies
#that the command executed without problem. A value of 0 denotes that
#the command could not bew executed due to some error.
#
#The second element of the response is the length in characters of the
#third element of the response. This value may be 0, in which case
#there is no third element.
#
#The third element is a text string that depends on the type of the
#command and/or whether an error occurred. If an error occurs, the
#third element will contain a clarifying description of the error if
#any. 
#
#The commands the Serif Server accepts are:
#
#     ANALYZE\n<DOCUMENT>: The Serif Server is to analyze the
#          <DOCUMENT> and return the results.
#
#     SHUTDOWN: The Serif Server is to shut down.
#
#If the returned status is 0 (ERROR), the error description is printed
#to STDERR, the client disconnects, closes the connection, and dies.
#Otherwise, this subroutine returns the response, if any.

sub sendCommand
{
    my ($sock, $command) = @_;
    my ($status, $response_length, $response, $size);

    $response = "";

    $size = length ($command);
    print $sock "$size $command";
    #print $command . "\n\n";

    $status = <$sock>;
    chomp $status;
    $response_length = <$sock>;
    chomp $response_length;

    if ($response_length > 0)
    {
	read($sock, $response, $response_length) or 
	    die "Couldn't read from socket.\n\n";
    }

    #for SHUTDOWN command, the server may close the
    #connection before the client gets the response - a condition we
    #really don't care about
    die "$command failed: $status $response\n\n" 
	unless (($status == $OK) || 
		($command eq "SHUTDOWN"));

    $response;
}

sub fileToString 
{
    my $file = shift;
    
    open(IN, "<$file") || return ('', 'OPEN ERROR');
    my $content = join('', <IN>);
    close(IN) || return ('', 'CLOSE ERROR');
    return ($content, 'OK');
}
