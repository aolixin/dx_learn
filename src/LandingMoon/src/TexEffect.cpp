#include "Effects.h"
#include <XUtil.h>
#include <RenderStates.h>
#include <EffectHelper.h>
#include <DXTrace.h>
#include <Vertex.h>
#include <TextureManager.h>
#include "LightHelper.h"

using namespace DirectX;

# pragma warning(disable: 26812)


class TexEffect::Impl
{
public:
	// 必须显式指定
	Impl() {}
	~Impl() = default;

public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::unique_ptr<EffectHelper> m_pEffectHelper;

	ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;

	std::shared_ptr<IEffectPass> m_pCurrEffectPass;
	ComPtr<ID3D11InputLayout> m_pCurrInputLayout;
	D3D11_PRIMITIVE_TOPOLOGY m_CurrTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
};

//
// TexEffect
//

namespace
{
	// TexEffect单例
	static TexEffect* g_pInstance = nullptr;
}

TexEffect::TexEffect()
{
	if (g_pInstance)
		throw std::exception("TexEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<TexEffect::Impl>();
}

TexEffect::~TexEffect()
{
}

TexEffect::TexEffect(TexEffect&& moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

TexEffect& TexEffect::operator=(TexEffect&& moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

TexEffect& TexEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("TexEffect needs an instance!");
	return *g_pInstance;
}


bool TexEffect::InitAll(ID3D11Device* device)
{
	if (!device)
		return false;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	// 创建顶点着色器
	pImpl->m_pEffectHelper->CreateShaderFromFile("TexVS", L"ShaderBin/Tex_VS.cso", device,
		nullptr, nullptr, nullptr, blob.GetAddressOf());
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::GetInputLayout(), ARRAYSIZE(VertexPosNormalTex::GetInputLayout()),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

	// 创建像素着色器
	pImpl->m_pEffectHelper->CreateShaderFromFile("TexPS", L"ShaderBin/Tex_PS.cso", device);


	// 创建通道
	EffectPassDesc passDesc;
	passDesc.nameVS = "TexVS";
	passDesc.namePS = "TexPS";
	HR(pImpl->m_pEffectHelper->AddEffectPass("Tex", device, &passDesc));

	pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSLinearWrap.Get());

	// 设置调试对象名
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "TexEffect.VertexPosNormalTexLayout");
#endif
	pImpl->m_pEffectHelper->SetDebugObjectName("TexEffect");

	return true;
}

void XM_CALLCONV TexEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV TexEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
	XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV TexEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
	XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void TexEffect::SetMaterial(const Material& material)
{
	TextureManager& tm = TextureManager::Get();

	PhongMaterial phongMat{};
	phongMat.ambient = material.Get<XMFLOAT4>("$AmbientColor");
	phongMat.diffuse = material.Get<XMFLOAT4>("$DiffuseColor");
	phongMat.diffuse.w = material.Get<float>("$Opacity");
	phongMat.specular = material.Get<XMFLOAT4>("$SpecularColor");
	phongMat.specular.w = material.Has<float>("$SpecularFactor") ? material.Get<float>("$SpecularFactor") : 1.0f;
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Material")->SetRaw(&phongMat);

	auto pStr = material.TryGet<std::string>("$Diffuse");
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", pStr ? tm.GetTexture(*pStr) : tm.GetNullTexture());
}

MeshDataInput TexEffect::GetInputData(const MeshData& meshData)
{
	MeshDataInput input;
	input.pInputLayout = pImpl->m_pCurrInputLayout.Get();
	input.topology = pImpl->m_CurrTopology;

	input.pVertexBuffers = {
		meshData.m_pVertices.Get(),
		meshData.m_pNormals.Get(),
		meshData.m_pTexcoordArrays.empty() ? nullptr : meshData.m_pTexcoordArrays[0].Get()
	};
	input.strides = { 12, 12, 8 };
	input.offsets = { 0, 0, 0 };

	input.pIndexBuffer = meshData.m_pIndices.Get();
	input.indexCount = meshData.m_IndexCount;

	return input;
}

void TexEffect::SetDirLight(uint32_t pos, const DirectionalLight& dirLight)
{
	auto buffer = pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLight");
	if (buffer == nullptr)return;
	buffer->SetRaw(&dirLight, (sizeof dirLight) * pos, sizeof dirLight);
}

void TexEffect::SetPointLight(uint32_t pos, const PointLight& pointLight)
{
	auto buffer = pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLight");
	if (buffer == nullptr)return;
	buffer->SetRaw(&pointLight, (sizeof pointLight) * pos, sizeof pointLight);
}

void TexEffect::SetSpotLight(uint32_t pos, const SpotLight& spotLight)
{
	auto buffer = pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLight");
	if (buffer == nullptr)return;
	buffer->SetRaw(&spotLight, (sizeof spotLight) * pos, sizeof spotLight);
}

void TexEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
	auto buffer = pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLight");
	if (buffer == nullptr)return;
	buffer->SetFloatVector(3, reinterpret_cast<const float*>(&eyePos));
}

void TexEffect::SetRenderDefault()
{
	pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("Tex");
	pImpl->m_pCurrInputLayout = pImpl->m_pVertexPosNormalTexLayout;
	pImpl->m_CurrTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void TexEffect::Apply(ID3D11DeviceContext* deviceContext)
{
	XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
	XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
	XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

	XMMATRIX VP = V * P;
	XMMATRIX WInvT = XMath::InverseTranspose(W);

	W = XMMatrixTranspose(W);
	VP = XMMatrixTranspose(VP);
	WInvT = XMMatrixTranspose(WInvT);

	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTranspose")->SetFloatMatrix(4, 4, (FLOAT*)&WInvT);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&VP);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_World")->SetFloatMatrix(4, 4, (FLOAT*)&W);

	if (pImpl->m_pCurrEffectPass)
		pImpl->m_pCurrEffectPass->Apply(deviceContext);
}


