#!/bin/sh
#
# osx_build.sh
# reaper_sws
#
# This script must be run in <sws-dev-root>/Install
#
# Created by Tim Payne on 4/18/10. Updated by Jeffos.
# Copyright 2010-2015 SWS. All rights reserved.
#
if [ ! -d output ]; then
  mkdir output
fi
# Build
cd ..
php reascript_vararg.php > reascript_vararg.h
xcodebuild clean -configuration Release
xcodebuild -configuration Release
read -p "Check build results, then press enter to continue..."
#
# Make Reascript wrappers
#
perl reascript_python.pl > sws_python32.py
perl reascript_python.pl -x64 > sws_python64.py
#
# Make installer
# Uses the pkg-dmg script available here: https://github.com/opichals/osx-pkg-dmg
chmod 777 Install/pkg-dmg
if [ -f Install/pkg-dmg ]; then
  Install/pkg-dmg --volname "SWS_Extension" --source sws_osx --symlink /Library/Application\ Support/REAPER/UserPlugins:/UserPlugins --copy Install/sws_osx_install.dsstore:/.DS_Store --copy FingersExtras/Grooves:/ --copy Install/sws_osx_install.py:/sws_python.py --copy sws_python32.py:/ --copy sws_python64.py:/ --license Install/license.txt --target Install/output/sws_osx.dmg --mkdir /.background --copy Install/sws_osx_install.tiff:/.background/backgroundImage.tiff --attribute V:/.background
fi  
#
# Upload (optional)
#
if [ -f Install/osx_upload.command ]; then
  Install/osx_upload.command
fi
