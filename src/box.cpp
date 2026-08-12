#define internal static

#include <d3d12.h>

#include "app.h"
#include "math.h"

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
  
};

int WINAPI WinMain(HINSTANCE hInstance,
		   HINSTANCE prevInstance,
		   PSTR cmdLine,
		   int showCmd)
{
  bool result = false;
  
  D3D* directx = new D3D;
  
  result = Initialize_Main_Window(directx);
  if (!result)
    return -1;
  
  result = Initialize_DirectX(directx);
  if (!result)
    return -1;

  Render(directx);

  int message = Run(directx);
  
  if (message == 0)
    {
      Release_DirectX(directx);
      delete directx;
    }
  
  return 0; 
}
