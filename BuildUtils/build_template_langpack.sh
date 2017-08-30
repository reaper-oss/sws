#!/bin/bash
# In order to build LangPack files:
# - this script must be performed from <development root>
# - the source tree must be in <development root>/sws/
sws/BuildUtils/Release/BuildLangpack.exe --template sws/*.rc sws/*.cpp sws/SnM/*.cpp sws/Autorender/*.cpp sws/Breeder/*.cpp sws/cfillion/*.cpp sws/Color/*.cpp sws/Console/*.cpp sws/Fingers/*.c?? sws/Freeze/*.cpp sws/IX/*.cpp sws/MarkerActions/*.cpp sws/MarkerList/*.cpp sws/Misc/*.cpp sws/nofish/*.cpp sws/ObjectState/*.cpp sws/Padre/*.cpp sws/Projects/*.cpp sws/Snapshots/*.cpp sws/snooks/*.cpp sws/TrackList/*.cpp sws/Utility/*.cpp sws/Wol/*.cpp sws/Xenakios/*.cpp > sws/Install/output/SWS_Template.ReaperLangPack
