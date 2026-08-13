#define internal static

#include <d3d12.h>

#include "math.h"
#include "app.h"
#include "box.h"

int WINAPI WinMain(HINSTANCE hInstance,
		   HINSTANCE prevInstance,
		   PSTR cmdLine,
		   int showCmd)
{
  bool result = false;
  
  D3D* directx = new D3D;
  directx->directx_instance = hInstance; 
  
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
