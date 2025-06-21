#include "GameApp.h"
#include <XUtil.h>
#include <DXTrace.h>

using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
	: D3DApp(hInstance, windowName, initWidth, initHeight)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	m_TextureManager.Init(m_pd3dDevice.Get());
	m_ModelManager.Init(m_pd3dDevice.Get());

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(m_pd3dDevice.Get());

	if (!m_BasicEffect.InitAll(m_pd3dDevice.Get()))
		return false;

	if (!InitResource())
		return false;

	return true;
}

void GameApp::OnResize()
{
	D3DApp::OnResize();

	m_pDepthTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);
	m_pDepthTexture->SetDebugObjectName("DepthTexture");

	// 摄像机变更显示
	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_BasicEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
	}
}

void GameApp::EarthRevolution(float dt) {
	// 每秒旋转 45 度（顺时针）
	auto deltaAngle = XM_PIDIV4 / 4 * dt;
	m_Angle += deltaAngle;
	if (m_Angle > XM_2PI)
		m_Angle -= XM_2PI;

	// 地球公转
	float x = m_sunOrbitRadius * sinf(m_Angle);
	float z = m_sunOrbitRadius * cosf(m_Angle);

	float y = 0.0f;


	Transform& earthTransform = m_earth.GetTransform();
	XMFLOAT3 oldEarthPos = earthTransform.GetPosition();
	XMVECTOR oldEarthVec = XMLoadFloat3(&oldEarthPos);

	XMFLOAT3 newEarthPos(x, y, z);
	XMVECTOR newEarthVec = XMLoadFloat3(&newEarthPos);

	auto deltaEarth = newEarthVec - oldEarthVec;
	earthTransform.SetPosition(newEarthPos);


	auto& planeTransform = m_plane.GetTransform();
	XMFLOAT3 oldPlanePos = planeTransform.GetPosition();
	XMVECTOR oldPlaneVec = XMLoadFloat3(&oldPlanePos);

	XMVECTOR newPlaneVec = oldPlaneVec + deltaEarth;
	XMFLOAT3 newPlanePos;
	XMStoreFloat3(&newPlanePos, newPlaneVec);
	planeTransform.SetPosition(newPlanePos);
}

void GameApp::PlaneMove(float dt) {

	auto& earthTransform = m_earth.GetTransform();
	auto& planeTransform = m_plane.GetTransform();

	auto earthPos = earthTransform.GetPosition();
	XMVECTOR earthVec = XMLoadFloat3(&earthPos);

	auto planePos = planeTransform.GetPosition();
	XMVECTOR planeVec = XMLoadFloat3(&planePos);

	XMVECTOR right = planeTransform.GetRightAxisXM();
	XMVECTOR earth2plane = planeVec - earthVec;
	XMVECTOR forward = planeTransform.GetForwardAxisXM();


	XMVECTOR moveDir = XMVectorZero();
	if (m_Keys['W'])
		moveDir += forward;
	if (m_Keys['S'])
		moveDir -= forward;
	if (m_Keys['A'])
		moveDir -= right;
	if (m_Keys['D'])
		moveDir += right;

	float moveSpeed = 10.0f;
	if (!XMVector3Equal(moveDir, XMVectorZero()))
	{
		XMVECTOR fromVec = XMVector3Normalize(planeVec - earthVec);

		planeVec = planeVec + moveDir * moveSpeed * dt;
		XMStoreFloat3(&planePos, planeVec);

		XMVECTOR toVec = XMVector3Normalize(planeVec - earthVec);

		// 旋转轴
		XMVECTOR axis = XMVector3Normalize(XMVector3Cross(fromVec, toVec));
		// 旋转角度
		float angle = XMVectorGetX(XMVector3AngleBetweenVectors(fromVec, toVec));

		planeTransform.RotateAroundXM(earthPos, axis
			, angle
		);
	}
}


void GameApp::UpdateScene(float dt)
{

	// 每秒旋转 45 度（顺时针）
	auto deltaAngle = XM_PIDIV4 / 4 * dt;
	m_Angle += deltaAngle;
	if (m_Angle > XM_2PI)
		m_Angle -= XM_2PI;

	// 地球公转
	float x = m_sunOrbitRadius * sinf(m_Angle);
	float z = m_sunOrbitRadius * cosf(m_Angle);

	float y = 0.0f;



	Transform& earthTransform = m_earth.GetTransform();
	XMFLOAT3 oldEarthPos = earthTransform.GetPosition();
	XMVECTOR oldEarthVec = XMLoadFloat3(&oldEarthPos);

	XMFLOAT3 newEarthPos(x, y, z);
	XMVECTOR newEarthVec = XMLoadFloat3(&newEarthPos);

	//XMFLOAT3 newEarthPos = oldEarthPos;
	//XMVECTOR newEarthVec = oldEarthVec;

	auto deltaEarth = newEarthVec - oldEarthVec;
	earthTransform.SetPosition(newEarthPos);


	auto& planeTransform = m_plane.GetTransform();
	XMFLOAT3 oldPlanePos = planeTransform.GetPosition();
	XMVECTOR oldPlaneVec = XMLoadFloat3(&oldPlanePos);

	XMVECTOR newPlaneVec = oldPlaneVec + deltaEarth;
	XMFLOAT3 newPlanePos;
	XMStoreFloat3(&newPlanePos, newPlaneVec);
	planeTransform.SetPosition(newPlanePos);



	// plane 移动

	XMVECTOR right = planeTransform.GetRightAxisXM();
	XMVECTOR earth2plane = newPlaneVec - newEarthVec;
	//XMVECTOR forward = XMVector3Normalize(XMVector3Cross(right, earth2plane));
	XMVECTOR forward = planeTransform.GetForwardAxisXM();


	XMVECTOR moveDir = XMVectorZero();
	if (m_Keys['W'])
		moveDir += forward;
	if (m_Keys['S'])
		moveDir -= forward;
	if (m_Keys['A'])
		moveDir -= right;
	if (m_Keys['D'])
		moveDir += right;

	float moveSpeed = 10.0f;
	if (!XMVector3Equal(moveDir, XMVectorZero()))
	{
		auto planeVec = newPlaneVec + moveDir * moveSpeed * dt;
		XMFLOAT3 planePos;

		XMStoreFloat3(&planePos, planeVec);

		//planeTransform.SetPosition(planePos);
		XMVECTOR earthVec = XMLoadFloat3(&newEarthPos);
		XMVECTOR fromVec = XMVector3Normalize(newPlaneVec - earthVec);
		XMVECTOR toVec = XMVector3Normalize(planeVec - earthVec);

		// 旋转轴
		XMVECTOR rotationAxis = XMVector3Normalize(XMVector3Cross(fromVec, toVec));
		XMFLOAT3 axis;
		XMStoreFloat3(&axis, rotationAxis);
		// 旋转角度
		float rotationAngle = XMVectorGetX(XMVector3AngleBetweenVectors(fromVec, toVec));

		planeTransform.RotateAround(newEarthPos, axis, rotationAngle);
	}


	// 摄像机
	auto planePos = planeTransform.GetPosition();
	XMVECTOR planeVec = XMLoadFloat3(&planePos);

	float distance = 3.0f;
	//XMVECTOR cameraVec = XMVectorAdd(planeVec, XMVectorScale(dir, distance));

	XMVECTOR cameraVec = planeTransform.GetForwardAxisXM() * -distance + planeVec;
	//cameraVec += planeTransform.GetUpAxisXM();

	XMFLOAT3 cameraPos;
	XMStoreFloat3(&cameraPos, cameraVec);

	//m_pCamera->SetPosition(cameraPos);
	XMVECTOR planeRightVec = planeTransform.GetRightAxisXM();
	XMVECTOR cameraToPlane = XMVector3Normalize(planeVec - cameraVec);
	XMVECTOR cameraUpVec = XMVector3Normalize(XMVector3Cross(cameraToPlane, planeRightVec));
	XMFLOAT3 cameraUp;
	XMStoreFloat3(&cameraUp, cameraUpVec);
	m_pCamera->LookAt(cameraPos, planeTransform.GetPosition(), cameraUp);
	if (this->pitch != 0.0f) {
		Transform& cameraTransform = m_pCamera->GetTransform();
		XMFLOAT3 planeRight;
		XMStoreFloat3(&planeRight, planeRightVec);
		cameraTransform.RotateAround(planeTransform.GetPosition(), planeRight, pitch);
	}

	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
	m_BasicEffect.SetEyePos(m_pCamera->GetPosition());
}


void GameApp::DrawScene()
{
	// 创建后备缓冲区的渲染目标视图
	if (m_FrameCount < m_BackBufferCount)
	{
		ComPtr<ID3D11Texture2D> pBackBuffer;
		m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer.GetAddressOf()));
		CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		m_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), &rtvDesc, m_pRenderTargetViews[m_FrameCount].ReleaseAndGetAddressOf());
	}


	float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_pd3dImmediateContext->ClearRenderTargetView(GetBackBufferRTV(), black);
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthTexture->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	ID3D11RenderTargetView* pRTVs[1] = { GetBackBufferRTV() };
	m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthTexture->GetDepthStencil());
	D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
	m_pd3dImmediateContext->RSSetViewports(1, &viewport);

	m_BasicEffect.SetRenderDefault();
	//m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	m_sun.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	m_earth.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	m_moon.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
	m_plane.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

	//ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}



bool GameApp::InitResource()
{
	// ******************
	// 初始化游戏对象
	//

	// sun
	Model* pModel = m_ModelManager.CreateFromFile("Resource\\Models\\sun.obj");
	m_sun.SetModel(pModel);
	pModel->SetDebugObjectName("sun");

	Transform& sunTransform = m_sun.GetTransform();
	sunTransform.SetPosition(0.0f, 0.0f, 0.0f);
	//sunTransform.SetScale(0.0f, 0.0f, 0.0f);

	// earth
	pModel = m_ModelManager.CreateFromFile("Resource\\Models\\earth.obj");
	m_earth.SetModel(pModel);
	pModel->SetDebugObjectName("earth");

	Transform& earthTransform = m_earth.GetTransform();
	earthTransform.SetPosition(0.0f, 0.0f, -1000.0f);


	pModel = m_ModelManager.CreateFromFile("Resource\\Models\\moon.obj");
	m_moon.SetModel(pModel);
	pModel->SetDebugObjectName("moon");

	Transform& moonTransform = m_moon.GetTransform();
	moonTransform.SetPosition(50.0f, 30.0f, -1000.0f);

	pModel = m_ModelManager.CreateFromFile("Resource\\Models\\plane.obj");
	m_plane.SetModel(pModel);
	pModel->SetDebugObjectName("plane");

	Transform& planeTransform = m_plane.GetTransform();
	planeTransform.SetPosition(0.0f, 30.0f, -1000.0f);

	// ******************
	// 初始化摄像机
	//

	auto camera = std::make_shared<FirstPersonCamera>();
	m_pCamera = camera;

	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);

	camera->SetPosition(0.0f, 10.0f, -1100.0f);
	camera->LookAt(camera->GetPosition(), XMFLOAT3(0, 0, 0), XMFLOAT3(0, 1, 0));

	//camera->SetTarget(earthTransform.GetPosition());
	//camera->SetDistance(10.0f);
	//m_pCamera->SetDistanceMinMax(0.0f, 20.0f);
	//camera->RotateX(0);

	camera->SetFrustum(XM_PI / 2, AspectRatio(), 1.0f, 100000.0f);

	m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
	m_BasicEffect.SetViewMatrix(camera->GetViewMatrixXM());
	m_BasicEffect.SetProjMatrix(camera->GetProjMatrixXM());
	m_BasicEffect.SetEyePos(camera->GetPosition());

	// 环境光
	DirectionalLight dirLight{};
	dirLight.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	m_BasicEffect.SetDirLight(0, dirLight);
	// 灯光
	PointLight pointLight{};
	pointLight.position = XMFLOAT3(0.0f, 20.0f, 0.0f);
	pointLight.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	pointLight.diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	pointLight.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	pointLight.att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	pointLight.range = 30.0f;
	m_BasicEffect.SetPointLight(0, pointLight);

	return true;
}




LRESULT GameApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	D3DApp::MsgProc(hwnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_KEYDOWN:
		m_Keys[wParam] = true;
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		break;

	case WM_KEYUP:
		m_Keys[wParam] = false;
		break;

	case WM_MOUSEMOVE:
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		if (prevMouseX >= 0 && prevMouseY >= 0) {
			int deltaX = xPos - prevMouseX;
			int deltaY = yPos - prevMouseY;

			float rotateAmountY = deltaX * 0.01f;
			float rotateAmountX = deltaY * 0.01f;
			this->pitch += rotateAmountX;

			auto& planeTransform = m_plane.GetTransform();


			planeTransform.RotateAround(m_earth.GetTransform().GetPosition(), planeTransform.GetUpAxis(), rotateAmountY);
		}

		prevMouseX = xPos;
		prevMouseY = yPos;

		break;
	}

	case WM_LBUTTONDOWN:
		// 左键按下
		break;

	case WM_RBUTTONDOWN:
		// 右键按下
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}