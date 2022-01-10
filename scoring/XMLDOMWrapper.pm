# Copyright BBN Technologies Corp. 2007
# All rights reserved


#
# Wrapper for XMLDOM parser. Handles parsing errors and warnings (complains and dies).
#

package XMLDOMWrapper;

use XML::DOM;

use File::Basename;

sub load {
    my( $file ) = @_;

    my $DOMThing = new XML::DOM::Parser(Style => 'Objects');
    
    die "Could not create DOMThing" unless $DOMThing;
    
    my($doc) = $DOMThing->parsefile("$file", ErrorContext => 3 ) || print STDERR "Error loading $file\n";
    
    return $doc;
}

sub loadXML {
    my( $XMLstring ) = @_;

    my $DOMThing = new XML::DOM::Parser(Style => 'Objects');
    
    die "Could not create DOMThing" unless $DOMThing;
    
    my($doc) = $DOMThing->parsefile( $XMLDOM , ErrorContext => 3 ) || print STDERR "Error parsing $XMLstring\n";
    
    return $doc;
}

1;

