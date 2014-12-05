#!/usr/bin/perl

use warnings;
use strict;

use Net::Amazon::EC2;
use Data::Dumper;

##################################################
# Global Constants
#

my $awsId    = "AKIAJSAWQNEHI5XHFA4Q";
my $awsKey   = "9x3MMw/b7/0WledPfhNoIrPw41c2XYWaxAhRr87x";
my $ami      = "ami-9e1982f6";
my $keyPair  = "josnet";
my $ec2 = Net::Amazon::EC2->new(
  AWSAccessKeyId => $awsId, 
  SecretAccessKey => $awsKey,
  region => "us-east-1",
  InstanceType => "m1.small"
);

##################################################
# Functions
#

sub get_dns {
  my $inst_id = shift;
  my $running_instances = $ec2->describe_instances;
   
  foreach my $reservation (@$running_instances) {
    foreach my $instance ($reservation->instances_set) {
      if($instance->instance_id eq $inst_id) {
        if($instance->dns_name) {
          return $instance->dns_name;
        }
      }
    }
  }

  return "";
}

##################################################
# Loop
#

print("Launching instance");
my $instance = $ec2->run_instances(
  ImageId => "$ami",
  KeyName => "$keyPair",
  MinCount => 1,
  MaxCount => 1);

my $cur_inst = $instance->instances_set->[0];
my $cur_inst_id = $cur_inst->instance_id;
my $cur_inst_dns = "";

# Wait for it to init.
while(length($cur_inst_dns = get_dns($cur_inst_id)) == 0) {
  sleep(1);
  print(".");
}
print("\n");
sleep(5);

print("New instance $cur_inst_id at $cur_inst_dns\n\t:: initializing");

while(0 != system("ssh -i $keyPair.pem -o StrictHostKeyChecking=no -q -l ubuntu $cur_inst_dns ls /boot/grub/menu.lst")) {
  sleep(1);
  print(".");
}
print("\n");

do {
    print("You done yet? [y/N] ");
} until(readline() =~ /[yY]/);

print("\nRebooting $cur_inst_id into JOS\n");
$ec2->reboot_instances(InstanceId => "$cur_inst_id");

do {
    print("You done yet? [y/N] ");
} until(readline() =~ /[yY]/);

print("\nTerminating $cur_inst_id\n");

# Terminate instance

my $result = $ec2->terminate_instances(InstanceId => $cur_inst_id);

0;

