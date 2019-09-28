set BuildArch=x86

rem call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64 8.1
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat" -arch=amd64
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" -arch=x86
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=%BuildArch%

rem set project_style="Visual Studio 14 2015 Win64"
rem set project_style="Visual Studio 15 2017 Win64"
rem set project_style="Visual Studio 15 2017"

if %BuildArch% == x86 goto x86

set project_style="Visual Studio 16 2019 Win64"
set project_arch="x64"
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64

goto end

:x86
set project_style="Visual Studio 16 2019"
set project_arch="Win32"
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x86

:end
