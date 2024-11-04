#! /usr/bin/env perl
use strict;
use POSIX qw(strftime);
my ($dev,$speed)=@ARGV;
if (not defined $dev){
    die "usage: $0 dev"
}
if (! -e $dev) {
    die "$dev not found"
}
open(my $fh,"<",$dev) or die "unable to open $dev";
if (defined $speed){
    print("setting speed $speed");
    system("stty speed $speed < $dev") == 0 or die "unable to set speed";    
}
my $last=0;
while (<$fh>){
    my $x=time();
    if ($last != 0){
        if ($x > ($last+5)){
            print("****gap***\n");
        }
    }
    printf strftime("%Y/%m/%d-%H%M%S",localtime($x));
    printf("[%04.2f]: ",$x-$last); 
    $last=$x;
    print $_;
}