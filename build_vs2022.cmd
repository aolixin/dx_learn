@echo on

cd src

del /S /Q *.cso


cmake --preset "vs2022 - amd64"

cmake --build --preset "vs2022 - amd64-debug"

cmake --build --preset "vs2022 - amd64-release"

cd ..

xcopy /E /I /Y src\build\bin\Debug bin\Debug\

xcopy /E /I /Y src\build\bin\Release bin\Release\

pause
