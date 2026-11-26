@echo off
rem Compiles the stdshader_dx9 .vcs files with this tree's shadercompile (x64), following
rem materialsystem/stdshaders/buildshaders.bat for the PC lists, without Perforce or VMPI workers.
rem usage: tools\build_shaders.bat [list ...]     (default: stdshader_dx9_20b stdshader_dx9_20b_new stdshader_dx9_30)
rem output: .port-build\shaders\fxc\*.vcs  (stage_win.sh copies them to platform\shaders\fxc)
setlocal enabledelayedexpansion
for %%I in ("%~dp0..") do set ROOT=%%~fI
set SRC=%ROOT%\src
set B=%ROOT%\.port-build\win-x64
set BIN=%ROOT%\.port-build\shaderbuild\bin
set OUT=%ROOT%\.port-build\shaders
set PERL=C:\Strawberry\perl\bin\perl.exe
set PERL5LIB=%SRC%\devtools\perllib
rem the makefiles call plain "perl": make sure that is Strawberry, not Git's msys perl
set PATH=C:\Strawberry\perl\bin;%PATH%

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul || exit /b 1

if not exist "%BIN%" mkdir "%BIN%"
for %%F in (utils\shadercompile_launcher\shadercompile.exe utils\shadercompile\shadercompile_dll.dll utils\dx_proxy\dx_proxy.dll tier0\tier0.dll vstdlib\vstdlib.dll filesystem\filesystem_stdio.dll) do copy /y "%B%\%%F" "%BIN%\" >nul || exit /b 1
if not exist "%OUT%\fxc" mkdir "%OUT%\fxc"

rem COMBOLIST=<file> limits the build to the static combos listed there (the engine's -shadercensus output)
set COMBOARG=
if defined COMBOLIST set COMBOARG=-combolist "%COMBOLIST%"
set LISTS=%*
if "%LISTS%"=="" set LISTS=stdshader_dx9_20b stdshader_dx9_20b_new stdshader_dx9_30

pushd "%SRC%\materialsystem\stdshaders"
for %%L in (%LISTS%) do (
	echo ==== %%L
	set DIRECTX_FORCE_MODEL=
	set DIRECTX_SDK_VER=pc09.00
	if /i "%%L"=="stdshader_dx9_20b_new" set DIRECTX_SDK_VER=pc09.30
	if /i "%%L"=="stdshader_dx9_30" ( set DIRECTX_SDK_VER=pc09.30& set DIRECTX_FORCE_MODEL=30)
	if /i "%%L"=="fstop_scene_30" ( set DIRECTX_SDK_VER=pc09.30& set DIRECTX_FORCE_MODEL=30)
	if /i "%%L"=="fstop_census_20b_new" set DIRECTX_SDK_VER=pc09.30
	if /i "%%L"=="fstop_census_30" ( set DIRECTX_SDK_VER=pc09.30& set DIRECTX_FORCE_MODEL=30)
	if not exist shaders\fxc mkdir shaders\fxc
	for %%T in (filelist.txt filestocopy.txt filelistgen.txt inclist.txt vcslist.txt makefile.%%L.copy) do if exist %%T del /f /q %%T
	"%PERL%" "%SRC%\devtools\bin\updateshaders.pl" -source "%SRC%" %%L || exit /b 1
	rem the makefiles do not know the combo list: rebuild the limited .vcs files whose entries in it changed
	if defined COMBOLIST "%PERL%" "%ROOT%\tools\combolist_stamp.pl" check "%COMBOLIST%" makefile.%%L.copy || exit /b 1
	nmake /S /C -f makefile.%%L || exit /b 1
	if exist filelistgen.txt copy /y filelistgen.txt filelist.txt >nul
	rem filestocopy.txt (from nmake) lists the shader sources and includes; the workers also need the binaries
	(for %%F in (shadercompile.exe shadercompile_dll.dll dx_proxy.dll tier0.dll vstdlib.dll) do echo ..\..\..\.port-build\shaderbuild\bin\%%F) >> filestocopy.txt
	type filestocopy.txt | "%PERL%" "%SRC%\devtools\bin\uniqifylist.pl" > uniquefilestocopy.txt
	if exist filelist.txt (
		set HERE=!CD!
		pushd "%BIN%"
		"%BIN%\shadercompile.exe" -nompi -allowdebug !COMBOARG! -shaderpath "!HERE!" || exit /b 1
		popd
	)
	rem the engine only loads a .vcs whose header carries the CRC of its source (DYNAMIC_SHADER_COMPILE)
	if exist makefile.%%L.copy "%PERL%" "%ROOT%\tools\patch_vcs_crc.pl" makefile.%%L.copy || exit /b 1
	if defined COMBOLIST if exist makefile.%%L.copy "%PERL%" "%ROOT%\tools\combolist_stamp.pl" stamp "%COMBOLIST%" makefile.%%L.copy || exit /b 1
)
copy /y shaders\fxc\*.vcs "%OUT%\fxc\" >nul
popd
for /f %%N in ('dir /b "%OUT%\fxc\*.vcs" ^| %SystemRoot%\System32\find.exe /c /v ""') do echo built %%N vcs files in %OUT%\fxc
