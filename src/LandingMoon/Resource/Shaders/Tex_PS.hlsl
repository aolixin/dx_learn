#include "Basic.hlsli"

// 像素着色器(3D)
float4 PS(VertexPosHWNormalTex pIn) : SV_Target
{
    float4 texColor = g_DiffuseMap.Sample(g_Sam, pIn.tex);
    return texColor;
}
