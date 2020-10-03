@echo off
set ASTYLEEXE=c:\bin\astyle.exe
set ASTYLEOPT=-n -Q --options=..\astyle-cpp-options
%ASTYLEEXE% %ASTYLEOPT% *.h --exclude=targetver.h --exclude=stdafx.h
%ASTYLEEXE% %ASTYLEOPT% *.cpp --exclude=stdafx.cpp
