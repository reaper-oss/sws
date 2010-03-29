#!/bin/sh
#
# sws_osx_installer.sh
# reaper_sws
#
# Created by Tim Payne on 10/30/09.
# Copyright 2010 SWS. All rights reserved.
#
cd /Volumes/Dev/sws-extension
../osx-install/pkg-dmg --source sws_osx --copy sws_osx_install.dsstore:/.DS_Store --copy sws_osx_install.png:.sws_osx_install.png --copy maclicense.txt:license.txt --license maclicense.txt --target sws_osx.dmg
ftp -u ftp://standingwaterstudios@ftp.standingwaterstudios.com/www/reaper/sws_osx.dmg sws_osx.dmg
