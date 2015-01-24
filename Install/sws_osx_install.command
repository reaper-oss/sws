#!/bin/bash

function usage() { cat <<HELP
Usage: $(basename "$0") [INSTALL_PATH]

INSTALL_PATH defaults to ~/Library/Application Support/REAPER, which you may
need to change if you have a portable REAPER installation.

The UserPlugins and Scripts directories will be created inside INSTALL_PATH
if they don't already exist.
HELP
}

if [[ "$1" == "-h" || "$1" == "--help" ]]; then usage; exit; fi

src_dir="/Volumes/SWS_Extension"
reaper_dir="${1:-$HOME/Library/Application Support/REAPER}"

if [[ ! -d "$reaper_dir" ]]; then
  echo -e "ERROR: Directory '$reaper_dir' not found.\n"
  usage
  exit 1
fi

plugins_dir="$reaper_dir/UserPlugins"
scripts_dir="$reaper_dir/Scripts"

[[ -d "$plugins_dir" ]] || mkdir "$plugins_dir"

cp "$src_dir/reaper_sws.dylib" "$plugins_dir/reaper_sws.dylib"
cp "$src_dir/.whatsnew.txt" "$plugins_dir/reaper_sws_whatsnew.txt"

[[ -d "$scripts_dir" ]] || mkdir -p "$scripts_dir"

cp "$src_dir/.sws_python.py" "$scripts_dir/sws_python.py"
cp "$src_dir/.sws_python32.py" "$scripts_dir/sws_python32.py"
cp "$src_dir/.sws_python64.py" "$scripts_dir/sws_python64.py"
