@echo off
setlocal enabledelayedexpansion
echo TODOS:
findstr -s -l -n -i "TODO" src/*.cpp src/*.h

echo:
echo:
echo:

echo NOTES:
findstr -s -l -n -i "NOTE" src/*.cpp src/*.h

echo:
echo:
echo:

echo GLOBALS:
findstr -s -l -n -i "global_variable" src/*.cpp src/*.h

echo:
echo:
echo:

echo CONSTS:
findstr -s -l -n -i "const" src/*.cpp src/*.h