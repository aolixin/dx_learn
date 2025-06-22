#ifndef EFFECTS_H
#define EFFECTS_H

#include <IEffect.h>
#include <Material.h>
#include <MeshData.h>
#include <LightHelper.h>

class TexEffect : public IEffect, public IEffectTransform,
    public IEffectMaterial, public IEffectMeshData
{
public:
    TexEffect();
    virtual ~TexEffect() override;

    TexEffect(TexEffect&& moveFrom) noexcept;
    TexEffect& operator=(TexEffect&& moveFrom) noexcept;

    // 获取单例
    static TexEffect& Get();

    // 初始化所需资源
    bool InitAll(ID3D11Device* device);

    //
    // IEffectTransform
    //

    void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
    void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
    void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;

    //
    // IEffectMaterial
    //

    void SetMaterial(const Material& material) override;

    //
    // IEffectMeshData
    //

    MeshDataInput GetInputData(const MeshData& meshData) override;


    //
    // BasicEffect
    //

    // 默认状态来绘制
    void SetRenderDefault();
    
    // 各种类型灯光允许的最大数目
    static const int maxLights = 5;

    void SetDirLight(uint32_t pos, const DirectionalLight& dirLight);
    void SetPointLight(uint32_t pos, const PointLight& pointLight);
    void SetSpotLight(uint32_t pos, const SpotLight& spotLight);

    void SetEyePos(const DirectX::XMFLOAT3& eyePos);

    // 应用常量缓冲区和纹理资源的变更
    void Apply(ID3D11DeviceContext* deviceContext) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};


#endif
