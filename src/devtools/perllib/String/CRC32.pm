# Minimal stand-in for CPAN's String::CRC32, used by the Valve shader scripts (updateshaders.pl and friends).
# Same CRC-32 (IEEE 802.3, as in zlib), taken from the core Compress::Zlib module.
package String::CRC32;
use strict;
use warnings;
use Compress::Zlib ();
use Exporter 'import';
our @EXPORT = qw(crc32);
our $VERSION = '1.0';

sub crc32
{
	my ( $data, $init ) = @_;
	return defined $init ? Compress::Zlib::crc32( $data, $init ) : Compress::Zlib::crc32( $data );
}

1;
