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

void LockMouseToWindow(HWND hwnd)
{
	RECT rect;
	// 获取窗口客户区域（相对于窗口左上角）
	GetClientRect(hwnd, &rect);

	// 将客户区域坐标转换为屏幕坐标
	POINT ul = { rect.left, rect.top };
	POINT lr = { rect.right, rect.bottom };

	ClientToScreen(hwnd, &ul);
	ClientToScreen(hwnd, &lr);

	RECT clipRect = { ul.x - 2, ul.y - 2, lr.x + 2, lr.y + 2 };
	ClipCursor(&clipRect);  // 限制鼠标在 clipRect 区域
}


bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	m_TextureManager.Init(m_pd3dDevice.Get());
	m_ModelManager.Init(m_pd3dDevice.Get());

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(m_pd3dDevice.Get());

	if (!m_TexEffect.InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_SkyboxEffect.InitAll(m_pd3dDevice.Get()))
		return false;

	if (!InitResource())
		return false;

	ShowCursor(FALSE);
	LockMouseToWindow(m_hMainWnd);

	return true;
}

void GameApp::OnResize()
{
	D3DApp::OnResize();

	m_pDepthTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);
	m_pDepthTexture->SetDebugObjectName("DepthTexture");

	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_TexEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
		m_SkyboxEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
	}
}

void GameApp::EarthRevolution(float dt) {
	auto deltaAngle = deltaEarthRevolution * dt;
	m_earthRevolutionAngle += deltaAngle;
	if (m_earthRevolutionAngle > XM_2PI)
		m_earthRevolutionAngle -= XM_2PI;

	float x = m_sunOrbitRadius * sinf(m_earthRevolutionAngle);
	float z = m_sunOrbitRadius * cosf(m_earthRevolutionAngle);

	float y = 0.0f;


	Transform& earthTransform = m_earth.GetTransform();
	XMFLOAT3 oldEarthPos = earthTransform.GetPosition();
	XMVECTOR oldEarthVec = XMLoadFloat3(&oldEarthPos);

	XMFLOAT3 newEarthPos(x, y, z);
	XMVECTOR newEarthVec = XMLoadFloat3(&newEarthPos);

	auto deltaEarth = newEarthVec - oldEarthVec;
	earthTransform.SetPosition(newEarthPos);

	if (m_PlaneMovePos == EPlaneMovePos::Earth) {
		auto& planeTransform = m_plane.GetTransform();
		XMFLOAT3 oldPlanePos = planeTransform.GetPosition();
		XMVECTOR oldPlaneVec = XMLoadFloat3(&oldPlanePos);

		XMVECTOR newPlaneVec = oldPlaneVec + deltaEarth;
		XMFLOAT3 newPlanePos;
		XMStoreFloat3(&newPlanePos, newPlaneVec);
		planeTransform.SetPosition(newPlanePos);
	}
}

void GameApp::MoonRevolution(float dt)
{
	auto deltaAngle = deltaEarthRevolution * dt;
	m_moonRevolutionAngle += deltaAngle;
	if (m_moonRevolutionAngle > XM_2PI)
		m_moonRevolutionAngle -= XM_2PI;

	// 地球世界坐标
	XMFLOAT3 earthPos = m_earth.GetTransform().GetPosition();

	float moonAngle = m_moonRevolutionAngle * 12.0f;

	float moonRadius = 50.0f;

	float mx = EarthMoonOrbitRadiusA * cosf(moonAngle) + EarthMoonOrbitRadiusC;
	float mz = EarthMoonOrbitRadiusB * sinf(moonAngle);
	float my = 0;

	float tilt = m_inclinationAngle;
	float xRot = mx * cosf(tilt);
	float yRot = mx * sinf(tilt);


	XMFLOAT3 moonPos = {
		earthPos.x - xRot,
		earthPos.y - yRot,
		earthPos.z - mz
	};

	Transform& moonTransform = m_moon.GetTransform();
	XMVECTOR oldMoonVec = moonTransform.GetPositionXM();
	moonTransform.SetPosition(moonPos);
	XMVECTOR newMoonVec = moonTransform.GetPositionXM();

	if (m_PlaneMovePos == EPlaneMovePos::Moon) {
		auto& planeTransform = m_plane.GetTransform();
		XMFLOAT3 oldPlanePos = planeTransform.GetPosition();
		XMVECTOR oldPlaneVec = XMLoadFloat3(&oldPlanePos);

		auto deltaMoon = newMoonVec - oldMoonVec;

		XMVECTOR newPlaneVec = oldPlaneVec + deltaMoon;
		XMFLOAT3 newPlanePos;
		XMStoreFloat3(&newPlanePos, newPlaneVec);
		planeTransform.SetPosition(newPlanePos);
	}
}


void GameApp::PlaneMove(float dt) {
	if (m_PlaneMovePos == EPlaneMovePos::Earth) {
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
		if (rawToRotate != 0.0f)
		{
			planeTransform.RotateAround(planeTransform.GetPosition(), planeTransform.GetUpAxis(), rawToRotate);
			rawToRotate = 0.0f;
		}
	}
	else if (m_PlaneMovePos == EPlaneMovePos::EarthOrbit) {

		auto& planeTransform = m_plane.GetTransform();
		if (m_Keys['W'])
		{
			auto deltaAngle = deltaEarthRevolution * 12 * dt;
			m_planeEarthRevolutionAngle += deltaAngle;
			m_planeEarthRevolutionAngle = m_planeEarthRevolutionAngle <= XM_2PI ?
				m_planeEarthRevolutionAngle : m_planeEarthRevolutionAngle - XM_2PI;
		}
		if (m_Keys['S'])
		{
			auto deltaAngle = deltaEarthRevolution * 12 * dt;
			m_planeEarthRevolutionAngle -= deltaAngle;
			m_planeEarthRevolutionAngle = m_planeEarthRevolutionAngle >= -XM_2PI ?
				m_planeEarthRevolutionAngle : m_planeEarthRevolutionAngle + XM_2PI;
		}
		XMFLOAT3 pos = m_earth.GetTransform().GetPosition();
		float mx = m_earthOrbitRadius * cosf(m_planeEarthRevolutionAngle);
		float mz = m_earthOrbitRadius * sinf(m_planeEarthRevolutionAngle);
		float my = 0;

		float tilt = m_inclinationAngle;
		float xRot = mx * cosf(tilt);
		float yRot = mx * sinf(tilt);
		m_plane.GetTransform().SetPosition(pos.x - xRot, pos.y - yRot, pos.z - mz);

		if (rawToRotate != 0.0f)
		{
			planeTransform.RotateAround(planeTransform.GetPosition(), planeTransform.GetUpAxis(), rawToRotate);
			rawToRotate = 0.0f;
		}
	}
	else if (m_PlaneMovePos == EPlaneMovePos::EarthMoonOrbit) {
		// 飞行器在地月轨道运动
		auto& planeTransform = m_plane.GetTransform();
		XMFLOAT3 earthPos = m_earth.GetTransform().GetPosition();

		if (m_Keys['W'])
		{
			m_planeEarthMoonRevolutionAngle += deltaEarthRevolution * 12 * dt;
			if (m_planeEarthMoonRevolutionAngle > XM_2PI)
				m_planeEarthMoonRevolutionAngle -= XM_2PI;
		}
		if (m_Keys['S'])
		{
			m_planeEarthMoonRevolutionAngle -= deltaEarthRevolution * 12 * dt;
			if (m_planeEarthMoonRevolutionAngle < -XM_2PI)
				m_planeEarthMoonRevolutionAngle += XM_2PI;
		}

		float mx = EarthMoonOrbitRadiusA * cosf(m_planeEarthMoonRevolutionAngle) + EarthMoonOrbitRadiusC;
		float mz = EarthMoonOrbitRadiusB * sinf(m_planeEarthMoonRevolutionAngle);
		float tilt = m_inclinationAngle;
		float xRot = mx * cosf(tilt);
		float yRot = mx * sinf(tilt);
		planeTransform.SetPosition(earthPos.x - xRot, earthPos.y - yRot, earthPos.z - mz);

		if (rawToRotate != 0.0f)
		{
			planeTransform.RotateAround(planeTransform.GetPosition(), planeTransform.GetUpAxis(), rawToRotate);
			rawToRotate = 0.0f;
		}
	}
	else if (m_PlaneMovePos == EPlaneMovePos::MoonOrbit) {
		auto& planeTransform = m_plane.GetTransform();
		XMFLOAT3 moonPos = m_moon.GetTransform().GetPosition();

		if (m_Keys['W'])
		{
			m_planeMoonRevolutionAngle += deltaEarthRevolution * 12 * dt;
			if (m_planeMoonRevolutionAngle > XM_2PI)
				m_planeMoonRevolutionAngle -= XM_2PI;
		}
		if (m_Keys['S'])
		{
			m_planeMoonRevolutionAngle -= deltaEarthRevolution * 12 * dt;
			if (m_planeMoonRevolutionAngle < -XM_2PI)
				m_planeMoonRevolutionAngle += XM_2PI;
		}

		float tilt = 0.0f;
		float mx = m_moonOrbitRadius * cosf(m_planeMoonRevolutionAngle);
		float mz = m_moonOrbitRadius * sinf(m_planeMoonRevolutionAngle);
		float xRot = mx * cosf(tilt);
		float yRot = mx * sinf(tilt);
		planeTransform.SetPosition(moonPos.x - xRot, moonPos.y - yRot, moonPos.z - mz);

		if (rawToRotate != 0.0f)
		{
			planeTransform.RotateAround(planeTransform.GetPosition(), planeTransform.GetUpAxis(), rawToRotate);
			rawToRotate = 0.0f;
		}
	}
	else if (m_PlaneMovePos == EPlaneMovePos::Moon) {
		auto& moonTransform = m_moon.GetTransform();
		auto& planeTransform = m_plane.GetTransform();

		XMFLOAT3 moonPos = moonTransform.GetPosition();
		XMVECTOR moonVec = XMLoadFloat3(&moonPos);

		XMFLOAT3 planePos = planeTransform.GetPosition();
		XMVECTOR planeVec = XMLoadFloat3(&planePos);

		XMVECTOR right = planeTransform.GetRightAxisXM();
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

		if (!XMVector3Equal(moveDir, XMVectorZero()))
		{
			XMVECTOR fromVec = XMVector3Normalize(planeVec - moonVec);

			float moveSpeed = 5.0f;
			moveDir = XMVector3Normalize(moveDir);
			XMVECTOR newPlaneVec = planeVec + moveDir * moveSpeed * dt;
			XMFLOAT3 newPlanePos;
			XMStoreFloat3(&newPlanePos, newPlaneVec);

			XMVECTOR toVec = XMVector3Normalize(XMLoadFloat3(&newPlanePos) - moonVec);

			XMVECTOR axis = XMVector3Normalize(XMVector3Cross(fromVec, toVec));
			float angle = XMVectorGetX(XMVector3AngleBetweenVectors(fromVec, toVec));

			planeTransform.RotateAroundXM(moonPos, axis, angle);
		}

		if (rawToRotate != 0.0f)
		{
			planeTransform.RotateAround(planeTransform.GetPosition(), planeTransform.GetUpAxis(), rawToRotate);
			rawToRotate = 0.0f;
		}
	}
}

void GameApp::CameraMove(float dt) {
	auto& planeTransform = m_plane.GetTransform();
	auto planePos = planeTransform.GetPosition();
	XMVECTOR planeVec = XMLoadFloat3(&planePos);

	XMVECTOR cameraVec = planeTransform.GetForwardAxisXM() * -distance + planeVec;

	XMFLOAT3 cameraPos;
	XMStoreFloat3(&cameraPos, cameraVec);

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

	//Transform& cameraTransform = m_pCamera->GetTransform();
	//cameraTransform.SetPosition(0.0f, 200.0f, -1300.0f);
	//cameraTransform.LookAt(m_earth.GetTransform().GetPosition()/*, XMFLOAT3(0.0f, 1.0f, 0.0f)*/);


	m_TexEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
	m_TexEffect.SetEyePos(m_pCamera->GetPosition());
}


void GameApp::UpdateScene(float dt)
{
	if (m_Keys['M'] || m_Keys['E']) {
		if (m_Keys['M'])
		{
			if (m_PlaneMovePos == EPlaneMovePos::EarthMoonOrbit) {
				if (m_plane.GetTransform().GetDistanceTo(m_moon.GetTransform()) > m_minDistanceToLeaveEarthMoonorbit)
					return;
			}
			m_PlaneMovePos = EPlaneMovePos(std::min((m_PlaneMovePos + 1), EPlaneMovePos::Count - 1));
			m_Keys['M'] = false;
		}
		if (m_Keys['E'])
		{
			if (m_PlaneMovePos == EPlaneMovePos::EarthMoonOrbit) {
				if (m_plane.GetTransform().GetDistanceTo(m_earth.GetTransform()) > m_minDistanceToLeaveEarthMoonorbit)
					return;
			}
			m_PlaneMovePos = EPlaneMovePos(std::max(m_PlaneMovePos - 1, 0));
			m_Keys['E'] = false;
		}

		if (m_PlaneMovePos == EPlaneMovePos::Earth) {
			XMFLOAT3 pos = m_earth.GetTransform().GetPosition();
			m_plane.GetTransform().SetPosition(pos.x, pos.y + m_earthRadius, pos.z);
			m_plane.GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
		}
		else if (m_PlaneMovePos == EPlaneMovePos::EarthOrbit) {
			m_planeEarthRevolutionAngle = 0.0f;
			XMFLOAT3 pos = m_earth.GetTransform().GetPosition();
			float mx = m_earthOrbitRadius * cosf(m_planeEarthRevolutionAngle);
			float mz = m_earthOrbitRadius * sinf(m_planeEarthRevolutionAngle);
			float my = 0;

			float tilt = m_inclinationAngle;
			float xRot = mx * cosf(tilt);
			float yRot = mx * sinf(tilt);
			m_plane.GetTransform().SetPosition(pos.x - xRot, pos.y - yRot, pos.z - mz);
			m_plane.GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
		}
		else if (m_PlaneMovePos == EPlaneMovePos::EarthMoonOrbit) {
			m_planeEarthMoonRevolutionAngle = 0.0f;
			XMFLOAT3 pos = m_earth.GetTransform().GetPosition();
			float mx = EarthMoonOrbitRadiusA * cosf(m_planeEarthMoonRevolutionAngle) + EarthMoonOrbitRadiusC;
			float mz = EarthMoonOrbitRadiusB * sinf(m_planeEarthMoonRevolutionAngle);
			float tilt = m_inclinationAngle;
			float xRot = mx * cosf(tilt);
			float yRot = mx * sinf(tilt);
			m_plane.GetTransform().SetPosition(pos.x - xRot, pos.y - yRot, pos.z - mz);
			m_plane.GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
		}
		else if (m_PlaneMovePos == EPlaneMovePos::MoonOrbit) {
			m_planeMoonRevolutionAngle = 0.0f;
			XMFLOAT3 pos = m_moon.GetTransform().GetPosition();
			float radius = 25.0f;
			float mx = radius * cosf(m_planeMoonRevolutionAngle);
			float mz = radius * sinf(m_planeMoonRevolutionAngle);
			float tilt = m_inclinationAngle;
			float xRot = mx * cosf(tilt);
			float yRot = mx * sinf(tilt);
			m_plane.GetTransform().SetPosition(pos.x - xRot, pos.y - yRot, pos.z - mz);
			m_plane.GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
		}
		else if (m_PlaneMovePos == EPlaneMovePos::Moon) {
			XMFLOAT3 pos = m_moon.GetTransform().GetPosition();
			m_plane.GetTransform().SetPosition(pos.x, pos.y + m_moonRadius, pos.z);
			m_plane.GetTransform().SetRotation(0.0f, 0.0f, 0.0f);
		}
	}


	EarthRevolution(dt);

	MoonRevolution(dt);

	PlaneMove(dt);

	CameraMove(dt);


	m_SkyboxEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
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

	m_TexEffect.SetRenderDefault();
	m_sun.Draw(m_pd3dImmediateContext.Get(), m_TexEffect);
	m_earth.Draw(m_pd3dImmediateContext.Get(), m_TexEffect);
	m_moon.Draw(m_pd3dImmediateContext.Get(), m_TexEffect);
	m_plane.Draw(m_pd3dImmediateContext.Get(), m_TexEffect);

	// 绘制天空盒
	m_SkyboxEffect.SetRenderDefault();
	m_Skybox.Draw(m_pd3dImmediateContext.Get(), m_SkyboxEffect);

	HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}



bool GameApp::InitResource()
{
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

	pModel = m_ModelManager.CreateFromFile("Resource\\Models\\spaceship.obj");
	m_plane.SetModel(pModel);
	pModel->SetDebugObjectName("plane");

	Transform& planeTransform = m_plane.GetTransform();
	planeTransform.SetPosition(0.0f, 30.0f, -1000.0f);
	planeTransform.SetScale(0.5f, 0.5f, 0.5f);

	pModel = m_ModelManager.CreateFromGeometry("Skybox", Geometry::CreateBox());
	pModel->SetDebugObjectName("Skybox");
	pModel->materials[0].Set<std::string>("$Skybox", "sky");
	m_Skybox.SetModel(pModel);

	// Daylight
	std::string filenameStr;
	std::vector<ID3D11ShaderResourceView*> pCubeTextures;
	ComPtr<ID3D11Texture2D> pTex;
	D3D11_TEXTURE2D_DESC texDesc;
	std::unique_ptr<TextureCube> pTexCube;
	{
		filenameStr = "Resource\\Textures\\sky0.png";
		for (size_t i = 0; i < 6; ++i)
		{
			filenameStr[filenameStr.size() - 5] = '0' + (char)i;
			pCubeTextures.push_back(m_TextureManager.CreateFromFile(filenameStr));
		}

		pCubeTextures[0]->GetResource(reinterpret_cast<ID3D11Resource**>(pTex.ReleaseAndGetAddressOf()));
		pTex->GetDesc(&texDesc);
		pTexCube = std::make_unique<TextureCube>(m_pd3dDevice.Get(), texDesc.Width, texDesc.Height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		pTexCube->SetDebugObjectName("sky");
		for (uint32_t i = 0; i < 6; ++i)
		{
			pCubeTextures[i]->GetResource(reinterpret_cast<ID3D11Resource**>(pTex.ReleaseAndGetAddressOf()));
			m_pd3dImmediateContext->CopySubresourceRegion(pTexCube->GetTexture(), D3D11CalcSubresource(0, i, 1), 0, 0, 0, pTex.Get(), 0, nullptr);
		}
		m_TextureManager.AddTexture("sky", pTexCube->GetShaderResource());
	}

	// ******************
	// 初始化摄像机
	//
	auto camera = std::make_shared<FirstPersonCamera>();
	m_pCamera = camera;

	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);

	camera->SetPosition(0.0f, 10.0f, -1100.0f);
	camera->LookAt(camera->GetPosition(), XMFLOAT3(0, 0, 0), XMFLOAT3(0, 1, 0));


	camera->SetFrustum(XM_PI / 2, AspectRatio(), 1.0f, 100000.0f);

	m_TexEffect.SetWorldMatrix(XMMatrixIdentity());
	m_TexEffect.SetViewMatrix(camera->GetViewMatrixXM());
	m_TexEffect.SetProjMatrix(camera->GetProjMatrixXM());
	m_TexEffect.SetEyePos(camera->GetPosition());

	m_SkyboxEffect.SetViewMatrix(camera->GetViewMatrixXM());
	m_SkyboxEffect.SetProjMatrix(camera->GetProjMatrixXM());

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
			this->rawToRotate += rotateAmountY;
		}

		prevMouseX = xPos;
		prevMouseY = yPos;

		// 获取窗口客户区大小
		int windowWidth = m_ClientWidth;
		int windowHeight = m_ClientHeight;

		bool wrapped = false;

		if (xPos <= 0) {
			xPos = windowWidth - 2;
			wrapped = true;
		}
		else if (xPos >= windowWidth - 1) {
			xPos = 1;
			wrapped = true;
		}

		/*if (yPos <= 0) {
			yPos = windowHeight - 2;
			wrapped = true;
		}
		else if (yPos >= windowHeight - 1) {
			yPos = 1;
			wrapped = true;
		}*/

		if (wrapped) {
			POINT pt = { xPos, yPos };
			ClientToScreen(hwnd, &pt);
			SetCursorPos(pt.x, pt.y);

			// 更新prevMouseX/Y为新的坐标，避免跳变
			prevMouseX = xPos;
			prevMouseY = yPos;
		}

		break;
	}



	case WM_LBUTTONDOWN:
		break;

	case WM_RBUTTONDOWN:
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_MOUSEWHEEL:
	{
		short delta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (delta < 0)
			distance = std::min(distance + 1, 10.0f);
		else if (delta > 0)
			distance = std::max(distance - 1, 2.0f);
		break;
	}

	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}