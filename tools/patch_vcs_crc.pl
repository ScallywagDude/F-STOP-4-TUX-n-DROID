# Writes the source CRC into each compiled .vcs, as devtools/bin/copyshaders.pl does in Valve's pipeline
# (without its Perforce checkout and copy to game/platform).
# The shaderapi in this tree is built with DYNAMIC_SHADER_COMPILE: it loads a .vcs only when the CRC in the
# header matches the shader source it can find, and otherwise compiles every combo from source at run time.
# usage (from materialsystem/stdshaders): perl patch_vcs_crc.pl makefile.<list>.copy [...]
use strict;
use String::CRC32;

sub ReadInputFileWithIncludes
{
	my ( $filename ) = @_;
	open( my $in, "<", $filename ) or die "can't open $filename\n";
	my $output = "";
	while ( my $line = <$in> )
	{
		if ( $line =~ m/\#include\s+\"(.*)\"/i )
		{
			$output .= ReadInputFileWithIncludes( $1 );
		}
		else
		{
			$output .= $line;
		}
	}
	close $in;
	return $output;
}

my $patched = 0;
foreach my $copyfile ( @ARGV )
{
	open( my $list, "<", $copyfile ) or die "can't open $copyfile\n";
	while ( my $entry = <$list> )
	{
		$entry =~ s,//.*,,g;
		$entry =~ s/[\r\n]//g;
		next if $entry =~ m/^\s*$/;
		my ( $source, $target ) = split( /-----/, $entry );
		next unless $source =~ m/\.fxc$/i;
		my $vcs = "shaders\\fxc\\$target.vcs";
		next unless -e $vcs;
		my $crc = crc32( ReadInputFileWithIncludes( $source ) );
		# ShaderHeader_t::m_nSourceCRC32 is the seventh uint32
		open( my $fp, "+<", $vcs ) or die "can't open $vcs\n";
		binmode( $fp );
		seek( $fp, 6 * 4, 0 );
		print $fp pack( "I", $crc );
		close $fp;
		$patched++;
	}
	close $list;
}
print "patched source CRC into $patched vcs files\n";
