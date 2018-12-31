del .\compiled\PtokaX.exe
del .\compiled\PtokaX-gui.exe
del .\compiled\PtokaX.pdb
del .\compiled\PtokaX-gui.pdb

svn log > PtokaX-svn-log.txt

call "%VS10COMNTOOLS%\..\..\VC\bin\vcvars32.bat"
"%VS100COMNTOOLS%..\ide\devenv" PtokaX-gui.sln /Rebuild "Release|Win32"

"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\signtool.exe" sign /a /d "PtokaX++ GUI x86" /du "http://flylinkdc.blogspot.com" /t http://timestamp.verisign.com/scripts/timstamp.dll ".\compiled\PtokaX-gui.exe"


rem call "%VS100COMNTOOLS%\..\..\VC\bin\vcvars32.bat"
rem "%VS100COMNTOOLS%..\ide\devenv" PtokaX.sln /Rebuild "Release|Win32"
rem "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Bin\signtool.exe" sign /a /d "PtokaX++ console x86" /du "http://flylinkdc.blogspot.com" /t http://timestamp.verisign.com/scripts/timstamp.dll ".\compiled\PtokaX.exe"

call src-gen-filename.bat -src
7z.exe a -r -t7z -m0=lzma -mx=9 -mfb=512 -md=1024m -ms=on %FILE_NAME%-src.7z -i@src-include.txt -x@src-exclude.txt -x!./compiled/*

SYMUPLOAD 70D71CA7-8FDF-41C4-9AA1-4ADB30527A54 0.5.2.524 0 .\compiled\PtokaX-gui.pdb
SYMUPLOAD 70D71CA7-8FDF-41C4-9AA1-4ADB30527A54 0.5.2.524 0 .\compiled\PtokaX-gui.exe

rem SYMUPLOAD 11B2CC9B-B1E9-4894-9AE6-6C4CB3DFDECA 0.5.2.524 0 .\compiled\PtokaX.pdb
rem SYMUPLOAD 11B2CC9B-B1E9-4894-9AE6-6C4CB3DFDECA 0.5.2.524 0 .\compiled\PtokaX.exe
