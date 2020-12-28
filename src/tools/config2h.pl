#!/usr/bin/perl -w
#
# $Id: config2h.pl,v 1.5 Broadcom SDK $
# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
# 


use strict;
use warnings;


######################################################
#
#         Global 
# 
######################################################

use constant { true => 1, false => 0 };

# config type for context
our $CONFIG_VAR_TYPE_UNDEF = "undef";
our $CONFIG_VAR_TYPE_STRING = "string";
our $CONFIG_VAR_TYPE_BOOL   = "bool";
our $CONFIG_VAR_TYPE_HEXDECIMAL = "hex";
our $CONFIG_VAR_TYPE_DECIMAL = "dec";

######################################################
#
# Copyright
#
######################################################
our $copy_right = "/*\n * \$Id\$\n *\n * \$Copyright: (c) 2014 Broadcom Corp.\n * All Rights Reserved.\$\n *\n */\n\n";


######################################################
#
#          Util
# 
######################################################


sub config_parse_var_is_decimal
{
  my $var = shift;
	if ($var =~ /^\d+$/) {
		  return true;
  }
  return false;
}

sub config_parse_var_is_hexdecimal
{
  my $var = shift;
  if ($var =~ /^0[xX][0-9A-Fa-f]+$/) {
		  return true;
  }
  return false;
	
}

sub config_parse_var_is_bool
{
  my $var = shift;
	if ($var =~ /^[01]$/) {
		  return true;
  }
  return false;
  	
}

sub config_parse_line_is_comment
{
   my $line = shift;
   my $ret = false; 
   if ($line =~ /^[\s\t]*#.*/) {
       $ret = true;
   }
   return $ret; 
}

sub config_parse_line_var
{
   my $line = shift;
   my %var;
   my $var_value; 
   $var{"type"} = $CONFIG_VAR_TYPE_UNDEF; 
  
   if ($line =~ /^[\s\t]*(\w+)[\s\t]*\?{0,1}=[\s\t]*(\w+)/) {
      
      $var{"name"} = $1;
      $var{"value"} = $2;
      $var_value = $2;

      if (!defined($var{"name"}) || !defined($var_value)) { 
      	  return undef;
      }

      if ($var{"name"} eq "" ||
          $var_value eq "") 
      { 
      	  return undef;
      }
      
      if (config_parse_var_is_bool($var_value)) {
          $var{"type"} = $CONFIG_VAR_TYPE_BOOL;
      } elsif (config_parse_var_is_decimal($var_value)) {
          $var{"type"} = $CONFIG_VAR_TYPE_DECIMAL;  	
      } elsif (config_parse_var_is_hexdecimal($var_value)) {
          $var{"type"} = $CONFIG_VAR_TYPE_HEXDECIMAL;  	   	
      } else {
          $var{"type"} = $CONFIG_VAR_TYPE_STRING;
      }
      #print "$line : " . $var{"name"} . " " . $var{"value"} . " " . $var{"type"} .  "\n";   
      return \%var;
   }  else {
   	  return undef;
   }
   
   
}

sub config_parse
{
   my $file_name = shift;
   my $row = 1;
   my $CONFIG_HANDLE;
   my $var;
   my @var_list;
   # open config file 
   open($CONFIG_HANDLE, '<:encoding(UTF-8)' , $file_name) or die("Can not open config file: $file_name ");
   
   # parse config file 
   while (my $line = <$CONFIG_HANDLE>) {
         
          $row = $row + 1;
          chomp $line;
          if (config_parse_line_is_comment($line) == true) {
              next; 
          }
          $var = config_parse_line_var($line);         
          
          if (defined($var)) {
              push @var_list, $var;
          } 
   }

   close($CONFIG_HANDLE); 
   return @var_list;
} 

sub config_to_header_file 
{
   my $config_file_name = shift;
   my $config_header_name = shift;
   my @var_list;
   my $HEADER_HANDLE;
   my $var;

  
   @var_list = config_parse($config_file_name);
   # open config file 
   open($HEADER_HANDLE, ">$config_header_name") or die("Can not open config file: $config_header_name ");

   print $HEADER_HANDLE "$copy_right";
   print $HEADER_HANDLE "/*\n";
   print $HEADER_HANDLE " *\n";
   print $HEADER_HANDLE " *  Auto generated base on .config. Please do not edit me \n";  
   print $HEADER_HANDLE " *\n";
   print $HEADER_HANDLE " */\n";
   foreach $var (@var_list)
   { 
     if ($var->{"type"} eq $CONFIG_VAR_TYPE_STRING) {
         	   printf $HEADER_HANDLE "#define %-30s \"%s\"\n", $var->{"name"}, $var->{"value"};
     } else {
         	   printf $HEADER_HANDLE "#define %-30s %s\n", $var->{"name"}, $var->{"value"};
     }
     
   } 
   
   close($HEADER_HANDLE);

}

config_to_header_file(".config", "conf.h");
