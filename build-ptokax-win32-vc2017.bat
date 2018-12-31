del .\compiled\PtokaX.exe
del .\compiled\PtokaX-gui.exe
del .\compiled\PtokaX.pdb
del .\compiled\PtokaX-gui.pdb

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
chcp 437
"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe" PtokaX-gui.sln /m /t:Rebuild /p:COnfiguration="Release" /p:Platform="Win32"


"C:\Program Files (x86)\Windows Kits\10\bin\10.0.17134.0\x86\signtool.exe" sign /v /d "PtokaX++ GUI x86" /du "http://flylinkdc.blogspot.com" /fd sha1 /t http://timestamp.verisign.com/scripts/timstamp.dll ".\compiled\PtokaX-gui.exe"
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.17134.0\x86\signtool.exe" sign /as /v /d "PtokaX++ GUI x86" /du "http://flylinkdc.blogspot.com" /fd sha256 /tr http://timestamp.comodoca.com/rfc3161 /td sha256 ".\compiled\PtokaX-gui.exe"


call src-gen-filename.bat -src
7z.exe a -r -t7z -m0=lzma -mx=9 -mfb=512 -md=1024m -ms=on %FILE_NAME%-src.7z -i@src-include.txt -x@src-exclude.txt -x!./compiled/*

rem SYMUPLOAD 70D71CA7-8FDF-41C4-9AA1-4ADB30527A54 0.5.2.524 0 .\compiled\PtokaX-gui.pdb
rem SYMUPLOAD 70D71CA7-8FDF-41C4-9AA1-4ADB30527A54 0.5.2.524 0 .\compiled\PtokaX-gui.exe

rem SYMUPLOAD 11B2CC9B-B1E9-4894-9AE6-6C4CB3DFDECA 0.5.2.524 0 .\compiled\PtokaX.pdb
rem SYMUPLOAD 11B2CC9B-B1E9-4894-9AE6-6C4CB3DFDECA 0.5.2.524 0 .\compiled\PtokaX.exe
