#define internal static

#include <d3d12.h>

#include "app.h"
#include "math.h"
#include "upload_buffer.h"

struct Vertex
{
  float3 pos;
  float4 color; 
};

struct ObjectConstants
{
  float4x4 WorldViewProj = Identity4x4(); 
};

struct Box
{
  ID3D12RootSignature  *m_RootSignature = nullptr;
  ID3D12DescriptorHeap *m_DescriptorHeap = nullptr;

  
};

int main()
{

  
  return 0; 
}
