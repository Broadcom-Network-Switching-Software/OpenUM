#!/usr/bin/perl -w

#
# $Id: mkheader.pl,v 1.4 Broadcom SDK $
# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2021 Broadcom Inc. All rights reserved.
#
# Add a header including checksum, file length and board to a binary file
# 
 
use integer;

#
# Check and parse command line parameters
#

if (@ARGV == 5) {
    ($in_file, $out_file, $majver, $minver, $build) = @ARGV
       or die "Usage: ".__FILE__." <input_file> <output_file> <maj> <min> <build>\n";
    
    open (IN_FILE, "< $in_file") or die "Cannot open input file $in_file!";
    binmode(IN_FILE);

    open (OUT_FILE, "> $out_file") or die "Cannot open output file $out_file!";
    binmode(OUT_FILE);

    read(IN_FILE, $buf, 0x10);
    print OUT_FILE $buf;

# Insert version into image at offset 0x10
    printf OUT_FILE "%01X%01X%01X0", $majver, $minver, $build;

    read(IN_FILE, $buf, 4);
#
# Copy remaining data
#
    while(read(IN_FILE, $buf, 256)) {
        print OUT_FILE $buf;
    }
    
    close(IN_FILE);
    close(OUT_FILE);
    exit;
}

die "Usage: ".__FILE__." <input_file> <output_file> <board_name> <maj> <min> <build> [image_id]\n" unless (@ARGV == 6 || @ARGV == 7);

if (@ARGV == 6) {
    ($in_file, $out_file, $board_name, $majver, $minver, $build) = @ARGV
        or die "Usage: ".__FILE__." <input_file> <output_file> <board_name> <maj> <min> <build>\n";
} else {
    ($in_file, $out_file, $board_name, $majver, $minver, $build, $image_id) = @ARGV
        or die "Usage: ".__FILE__." <input_file> <output_file> <board_name> <maj> <min> <build> <image_id>\n";
}

#
# Open files
#
open (IN_FILE, "< $in_file") or die "Cannot open input file $in_file!";
binmode(IN_FILE);

open (OUT_FILE, "> $out_file") or die "Cannot open output file $out_file!";
binmode(OUT_FILE);

#
# Insert header seal
#
print OUT_FILE "UMHD";

#
# Remember where we are (to copy later)
#
$pos = tell(IN_FILE);

#
# Calculate byte count and checksum
#
$flag = 0;
$sum = 0;
$count = 0;
while(read(IN_FILE, $buf, 1)) {
    if (($count%50) == 0) {
        $sum += ord($buf);
        $sum &= 0xFFFF;
    }
    $count++;
}

#
# Insert size and checksum
#

printf OUT_FILE "%08X%04X%04X", $count, $flag, $sum;

#
# Insert board name
#
printf OUT_FILE $board_name;

$count = length($board_name);
$rem = 32 - $count;
while($rem > 0) {
    printf OUT_FILE "\0";
    $rem--;
} 

#
# Insert version
#
if (@ARGV == 6) {
    printf OUT_FILE "%01X%01X%01X0", $majver, $minver, $build;
} else {
    printf OUT_FILE "%01X%01X%01X%01X", $majver, $minver, $build, $image_id;
}

$rem = 8;

while($rem > 0) {
    printf OUT_FILE "\0";
    $rem--;
}

#
# Seek back for remaining data
#
seek IN_FILE, $pos, 0;

#
# Copy remaining data
#
while(read(IN_FILE, $buf, 256)) {
    print OUT_FILE $buf;
}

#
# Close all files
#
close(OUT_FILE);
close(IN_FILE);

