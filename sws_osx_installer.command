#!/bin/sh
#
# sws_osx_installer.sh
# reaper_sws
#
# Created by Tim Payne on 10/30/09.
# Copyright 2010 SWS. All rights reserved.
#

# JF> build "Release" and "Release x86_64" targets in xcode for this to work. we could call xcodebuild here but I gotta run...

cd /Volumes/Dev/sws-extension
mkdir sws_osx_universal
rm sws_osx_universal/*
lipo sws_osx/reaper_sws.dylib sws_osx64/reaper_sws.dylib -create -o sws_osx_universal/reaper_sws.dylib

../osx-install/pkg-dmg --source sws_osx_universal --copy sws_osx_install.dsstore:/.DS_Store --copy sws_osx_install.png:.sws_osx_install.png --copy maclicense.txt:license.txt --license maclicense.txt --target sws_osx.dmg
ftp -u ftp://standingwaterstudios@ftp.standingwaterstudios.com/www/reaper/sws_osx.dmg sws_osx.dmg
