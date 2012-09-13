#!/bin/sh
#
if [ ! -d ~/Library/Application\ Support/REAPER/UserPlugins ]; then
  mkdir ~/Library/Application\ Support/REAPER/UserPlugins
fi
cd /Volumes/SWS_Extension
cp reaper_sws.dylib ~/Library/Application\ Support/REAPER/UserPlugins/reaper_sws.dylib
cp .whatsnew.txt ~/Library/Application\ Support/REAPER/UserPlugins/reaper_sws_whatsnew.txt
#
if [ ! -d ~/Library/Application\ Support/REAPER/Scripts ]; then
  mkdir ~/Library/Application\ Support/REAPER/Scripts
fi
cd /Volumes/SWS_Extension
cp .sws_python.py ~/Library/Application\ Support/REAPER/Scripts/sws_python.py
cp .sws_python32.py ~/Library/Application\ Support/REAPER/Scripts/sws_python32.py
cp .sws_python64.py ~/Library/Application\ Support/REAPER/Scripts/sws_python64.py
