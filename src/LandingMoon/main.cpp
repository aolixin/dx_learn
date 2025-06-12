#include "GameApp.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmd, int nShow)
{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(szCmd);
	UNREFERENCED_PARAMETER(nShow);


	GameApp theApp(hInstance, L"Meshes", 1280, 720);

	if (!theApp.Init())
		return 0;

	return theApp.Run();

}

