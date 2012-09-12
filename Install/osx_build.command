#!/bin/sh
#
# osx_build.sh
# reaper_sws
#
# Created by Tim Payne on 4/18/10. Updated by Jeffos on 9/11/12.
# Copyright 2010-2012 SWS. All rights reserved.
#
# Build
cd ..
xcodebuild -configuration "Release x86_64"
xcodebuild -configuration Release
read -p "Check build results, then press enter to continue..."
#
# Make universal binary
#
lipo sws_osx/reaper_sws.dylib sws_osx64/reaper_sws.dylib -create -o sws_osx/universal.dylib
rm sws_osx/reaper_sws.dylib
mv sws_osx/universal.dylib sws_osx/reaper_sws.dylib
#
# Make Reascript wrappers
#
perl reascript_python.pl > sws_python.py
perl reascript_python.pl -x64 > sws_python64.py
#
# Make installer
#
# Uses the pkg-dmg script available here: https://github.com/opichals/osx-pkg-dmg
if [ -f Install/pkg-dmg ]; then
  Install/pkg-dmg --volname "SWS_Extension" --source sws_osx --copy Install/sws_osx_install.dsstore:/.DS_Store --copy Install/maclicense.txt:license.txt --copy FingersExtras/Grooves:/ --copy sws_python.py:.sws_python.py --copy sws_python64.py:.sws_python64.py --copy whatsnew.txt:.whatsnew.txt --copy Install/sws_osx_install.command:Install.command --license Install/maclicense.txt --target Install/output/sws_osx.dmg --mkdir .background --copy Install/sws_osx_install.jpg:.background/backgroundImage.jpg --attribute V:.background
fi  
#
# Upload (optional)
#
if [ -f Install/osx_upload.command ]; then
  Install/osx_upload.command
fi
