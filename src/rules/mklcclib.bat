@echo off
rem All this because of a bug in lcclib...
rmdir /Q /S L_OBJS >nul:
mkdir L_OBJS
copy win32\*.obj L_OBJS >nul:
copy utility\*.obj L_OBJS >nul:
copy gui\*.obj L_OBJS >nul:
copy imgfmt\*.obj L_OBJS >nul:
copy libgif\*.obj L_OBJS >nul:
copy libjpeg\*.obj L_OBJS >nul:
copy libpng\*.obj L_OBJS >nul:
copy libz\*.obj L_OBJS >nul:
cd L_OBJS
lcclib /out:%1 *.obj
copy %1 .. >nul:
cd ..
rmdir /Q /S L_OBJS >nul:
echo %1 is ready.
