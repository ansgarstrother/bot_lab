#!/usr/bin/perl
#
# Produces a # (and changes)

$ARGC = $#ARGV + 1;

$definename = $ARGV[0];

$vers = `git describe --match "v[0-9]*"`;

$vers = trim($vers);

print "const char ".$definename."[] = \"$vers\";\n";

sub trim($)
{
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}
