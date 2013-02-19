#!/usr/bin/perl
#
# Produces a #

$ARGC = $#ARGV + 1;

if ($ARGC < 1)
{
    print ("Usage: svnvers DEFINENAME [svn-path]");
}

if ($ARGC == 2)
{
    chdir $ARGV[1];
}

$definename = $ARGV[0];

$_ = `svn info`;

/Revision: ([0-9]+)/ ;

$svnversion = $1;

$_ = `svn status`;

if (/^[^?].*/m)
{
    $status = "+changes";
}
else
{
    $status = "svn";
}

print "const char ".$definename."[] = \"$svnversion$status\";\n";

