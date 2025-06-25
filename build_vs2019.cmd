@echo on

cd src

del /S /Q *.cso


cmake --preset "vs2019 - amd64"

cmake --build --preset "vs2019 - amd64-debug"

cmake --build --preset "vs2019 - amd64-release"

cd ..

xcopy /E /I /Y src\build_vs2019\bin\Debug bin\Debug\

xcopy /E /I /Y src\build_vs2019\bin\Release bin\Release\

pause
