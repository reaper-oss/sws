mv -f sws-extension sws
sws_build_sample_langpack.exe --template sws/*.rc sws/*.cpp sws/SnM/*.cpp sws/Autorender/*.cpp sws/Color/*.cpp sws/Console/*.cpp sws/Fingers/*.c?? sws/Freeze/*.cpp sws/IX/*.cpp sws/MarkerActions/*.cpp sws/MarkerList/*.cpp sws/Misc/*.cpp sws/ObjectState/*.cpp sws/Padre/*.cpp sws/Projects/*.cpp sws/Snapshots/*.cpp sws/TrackList/*.cpp sws/Utility/*.cpp sws/Xenakios/*.cpp > sws/SWS_Template.ReaperLangPack
mv -f sws sws-extension

