
@echo off

gcc -D NO_C11_THREADS -I ../src/ ../test/test.c -o test.exe

if %errorlevel% neq 0 (
	echo failled to build
	exit /b 1
)

%cd%/test.exe

if %errorlevel% neq 0 (
	echo test failled
	exit /b 1
)

exit /b 0