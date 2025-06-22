#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <WinMin.h>
#include "d3dApp.h"
#include "Effects.h"
#include <Camera.h>
#include <RenderStates.h>
#include <GameObject.h>
#include <Texture2D.h>
#include <Buffer.h>
#include <ModelManager.h>
#include <TextureManager.h>
#include <windowsx.h>

class GameApp : public D3DApp
{

public:

	enum EPlaneMovePos {
		Earth = 0,
		EarthOrbit = 1,
		EarthMoonOrbit = 2,
		MoonOrbit = 3,
		Moon = 4,
		Count = 5
	};

	GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void EarthRevolution(float dt);
	void MoonRevolution(float dt);
	void PlaneMove(float dt);
	void CameraMove(float dt);

	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
	bool InitResource();
private:

	TextureManager m_TextureManager;
	ModelManager m_ModelManager;

	TexEffect m_TexEffect;
	//UnlitEffect m_TexEffect;

	std::unique_ptr<Depth2D> m_pDepthTexture;

	GameObject m_sun;
	GameObject m_earth;
	GameObject m_moon;
	GameObject m_plane;

	std::shared_ptr<FirstPersonCamera> m_pCamera;

	float m_inclinationAngle = - DirectX::XM_PIDIV2 / 3;
	float m_earthRevolutionAngle = 0.0f;
	float m_moonRevolutionAngle = 0.0f;
	float m_planeEarthRevolutionAngle = 0.0f;
	float m_planeEarthMoonRevolutionAngle = 0.0f;
	float m_planeMoonRevolutionAngle = 0.0f;

	float m_sunOrbitRadius = 1000.0f;
	float m_earthRadius = 30.0f;
	float m_moonRadius = 10.0f;
	float m_earthOrbitRadius = 50.0f;
	float EarthMoonOrbitRadiusA = 170.0f;
	float EarthMoonOrbitRadiusB = 150.0f;
	float EarthMoonOrbitRadiusC = 80.0f;
	float m_moonOrbitRadius = 15.0f;


	int prevMouseX = -1;  // 初始为无效值，表示尚未记录
	int prevMouseY = -1;

	int centerX = m_ClientWidth / 2;
	int centerY = m_ClientHeight / 2;


	float pitch = 0.0f;
	float rawToRotate = 0.0f;
	float distance = 3.0f;

	const float deltaEarthRevolution = DirectX::XM_PIDIV4 / 16;

	bool m_Keys[256] = { false };


	EPlaneMovePos m_PlaneMovePos = EPlaneMovePos::Earth;

};


#endif