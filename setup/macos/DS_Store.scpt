-- Generates a .DS_Store to customize the .dmg looks
on run argv
  set image_name to item 1 of argv

  tell application "Finder"
    tell disk image_name
      open
        set current view of container window to icon view
        set theViewOptions to the icon view options of container window
        set background picture of theViewOptions to file ".background:background.tiff"
        set arrangement of theViewOptions to not arranged
        set icon size of theViewOptions to 52

        -- make alias can't create a link to an nonexistent file?
        set currentDir to target of container window as alias
        do shell script "ln -s '/Library/Application Support/REAPER/UserPlugins' " ¬
          & quoted form of POSIX path of currentDir

        tell container window
          set sidebar   width   to 0
          set statusbar visible to false
          set toolbar   visible to false
          set the bounds        to { 150, 150, 714, 689 }

          set targetFile to "reaper_sws64.dylib"
          if not exists item targetFile
            set targetFile to "reaper_sws32.dylib"
          end if

          set position of item targetFile      to { 124, 85 }
          set position of item "UserPlugins"   to { 421, 85 }
          set position of item "sws_python.py" to { 234, 311 }
          set position of item "Grooves"       to { 234, 450 }
        end tell

        update without registering applications
      close
    end tell
  end tell
end run
