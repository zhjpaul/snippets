#!/usr/bin/env perl

# if your auth failed,
# try sudo apt-get install libauthen-sasl-perl

use warnings;
use strict;
use Net::SMTP;

sub sendMail { 
    my $toAddress = 'to_address';
    # take 163 mail as an example
    my $mailUser = 'your_address@163.com';
    my $mailPwd = 'your_pwd';
    my $mailServer = 'smtp.163.com';

    my $from = "From: $mailUser\n";
    my $subject = "Subject: a test\n";
    my $message = "hello world\n";

    my $smtp = Net::SMTP->new($mailServer);

    $smtp->auth($mailUser, $mailPwd) || die "Auth Error! $!";
    $smtp->mail($mailUser);
    $smtp->to($toAddress);

    $smtp->data();
    $smtp->datasend($from);
    $smtp->datasend($subject);
    $smtp->datasend($message);
    $smtp->dataend();

    $smtp->quit();
    print "send mail successfully!\n";
}

sendMail();

