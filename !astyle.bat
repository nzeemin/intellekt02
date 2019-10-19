rem c:\bin\astyle.exe -n -Q --options=astyle-cpp-options emubase\*.h emubase\*.cpp Util\*.h Util\*.cpp
c:\bin\astyle.exe -n -Q --options=astyle-cpp-options *.h --exclude=targetver.h --exclude=stdafx.h
c:\bin\astyle.exe -n -Q --options=astyle-cpp-options *.cpp --exclude=stdafx.cpp
