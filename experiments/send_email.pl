#!/bin/env perl
#
# A very simple command-line script to send mail via smtp.

use strict;
use warnings;
use Net::SMTP;
use MIME::Lite;
use Getopt::Long;

my ($subject, $from, $to);
my $SMTP="smtp.bbn.com";
GetOptions("subject=s" => \$subject,
	   "from=s"    => \$from,
	   "to=s"      => \$to,
           "smtp=s"    => \$SMTP);
defined($subject) || die "-subject required";
defined($from) || die "-from required";
defined($to) || die "-to required";

my $msg = MIME::Lite->new(
    From     => $from,
    To       => $to,
    Subject  => $subject,
    Data     => join("", <STDIN>)) or die "$!\n";
$msg->send('smtp', $SMTP) or die "$!\n";
