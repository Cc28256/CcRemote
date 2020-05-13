@echo off
echo ----------------------------------------------------
echo Press any key to delete all files with ending:
echo  *.aps *.idb *.ncp *.obj *.pch *.tmp *.sbr
echo ----------------------------------------------------
pause

del /F /Q /S  *.aps *.idb *.db *.ncp *.obj *.pch *.sbr *.tmp *.pdb *.bsc *.ilk *.sdf *.ncb *.dep *.ipch *.tlog *.dll *.exp


pause


