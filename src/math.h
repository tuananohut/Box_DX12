#ifndef MATH_H
#define MATH_H

#include <Windows.h>
#include <DirectXMath.h>
#include <cstdint>

using namespace DirectX;

typedef XMFLOAT3 float3;
typedef XMFLOAT4 float4;
typedef XMFLOAT4X4 float4x4; 

internal float4x4 Identity4x4()
{
  return
    {
      1.f, 0.f, 0.f, 0.f,
      0.f, 1.f, 0.f, 0.f,
      0.f, 0.f, 1.f, 0.f,
      0.f, 0.f, 0.f, 1.f,
    };
}

#endif
