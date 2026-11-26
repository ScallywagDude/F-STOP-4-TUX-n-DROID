# Keeps census-limited .vcs files in step with the combo list they were built from.
# The generated makefiles rebuild a .vcs only when its shader source changes, so a grown -combolist would otherwise
# leave the old, smaller .vcs in place and the engine fails on the combos added since.
# usage (from materialsystem/stdshaders):
#   perl combolist_stamp.pl check <combolist> makefile.<list>.copy [minstatic]
#       deletes each limited .vcs of that shader list whose combo list entries changed
#   perl combolist_stamp.pl stamp <combolist> makefile.<list>.copy [minstatic]
#       records the entries each limited .vcs of that list was built from
# A .vcs is limited when its header has at least <minstatic> static combos (shadercompile -combolistmin, default 1M).
use strict;

my ( $mode, $list, $copyfile, $min ) = @ARGV;
die "usage: combolist_stamp.pl check|stamp <combolist> makefile.<list>.copy [minstatic]\n" unless $mode && $list && $copyfile;
$min = 1000000 unless defined $min;

# the .vcs files this shader list builds ("source.fxc-----target" lines written by updateshaders.pl)
my @targets;
if ( open( my $c, "<", $copyfile ) )
{
	while ( my $line = <$c> )
	{
		$line =~ s,//.*,,;
		$line =~ s/[\r\n]//g;
		my ( $source, $target ) = split( /-----/, $line );
		push @targets, "shaders/fxc/$target.vcs" if $target;
	}
	close $c;
}

my %entries;
open( my $in, "<", $list ) or die "can't open $list\n";
while ( my $line = <$in> )
{
	my ( $kind, $name, $idx ) = split( ' ', $line );
	next unless defined $idx;
	push @{ $entries{ lc $name } }, $idx;
}
close $in;

my $changed = 0;
foreach my $vcs ( @targets )
{
	next unless -e $vcs;
	open( my $fp, "<", $vcs ) or next;
	binmode( $fp );
	my $header;
	read( $fp, $header, 12 );
	close $fp;
	my ( $version, $total, $dynamic ) = unpack( "lll", $header );
	$dynamic = 1 unless $dynamic;
	next if $total / $dynamic < $min;

	( my $name = lc $vcs ) =~ s,^.*/,,;
	$name =~ s/\.vcs$//;
	my $want = join( "\n", sort { $a <=> $b } @{ $entries{ $name } || [] } ) . "\n";
	( my $stampfile = $vcs ) =~ s/\.vcs$/.combolist/;

	if ( $mode eq "stamp" )
	{
		open( my $out, ">", $stampfile ) or die "can't write $stampfile\n";
		print $out $want;
		close $out;
		next;
	}

	my $have = "";
	if ( open( my $s, "<", $stampfile ) ) { local $/; $have = <$s>; close $s; }
	next if $have eq $want;
	unlink $vcs or die "can't delete $vcs\n";
	unlink $stampfile;
	print "combo list changed for $name: rebuilding\n";
	$changed++;
}
print "combo list: $changed limited shaders to rebuild\n" if $mode eq "check";
