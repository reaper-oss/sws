#!/bin/sh
#
# osx_build.sh
# reaper_sws
#
# Created by Tim Payne on 4/18/10.
# Copyright -2011 SWS. All rights reserved.
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
# Make installer
#
Install/pkg-dmg --source sws_osx --copy Install/sws_osx_install.dsstore:/.DS_Store --copy Install/sws_osx_install.png:.sws_osx_install.png --copy Install/maclicense.txt:license.txt --copy FingersExtras/Grooves:/ --license Install/maclicense.txt --target Install/output/sws_osx.dmg
#
# Upload
#
if [ -f Install/osx_upload.command ]; then
  Install/osx_upload.command
fi
