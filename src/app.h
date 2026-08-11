#ifndef APP_H
#define APP_H

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <d3d12.h>

#include "timer.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdi32.lib")

struct D3D
{
  HINSTANCE directx_instance = nullptr;
  HWND handle_window = nullptr;
  bool is_fullscreen = false;

  bool m4x_msaa_state = false;
  UINT m4x_msaa_quality = 0;

  Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;
  Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain;
  Microsoft::WRL::ComPtr<ID3D12Device> d3d12_device;

  Microsoft::WRL::ComPtr<ID3D12Fence> fence;
  UINT64 current_fence = 0;

  Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> direct_cmd_list_alloc;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;

  static const int swap_chain_buffer_count = 2;
  int current_back_buffer = 0;
  Microsoft::WRL::ComPtr<ID3D12Resource> swap_chain_buffer[swap_chain_buffer_count];
  Microsoft::WRL::ComPtr<ID3D12Resource> depth_stencil_buffer;

  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap;

  D3D12_VIEWPORT screen_viewport;
  D3D12_RECT scissor_rect;

  UINT rtv_descriptor_size = 0;
  UINT dsv_descriptor_size = 0;
  UINT cbv_srv_uav_descriptor_size = 0; 

  LPCWSTR window_caption = L"Box"; 
  D3D_DRIVER_TYPE d3d_driver_type = D3D_DRIVER_TYPE_HARDWARE;
  DXGI_FORMAT back_buffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
  DXGI_FORMAT depth_stencil_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  int width = 1080;
  int height = 720;

  Timer timer;
};

bool Initialize_Main_Window(D3D *directx);

bool Initialize_DirectX_App(D3D *directx, HINSTANCE instance);
void Release_DirectX(D3D *directx); 

bool Initialize_DirectX(D3D *directx);

int Run(D3D* directx);

void LogAdapters(D3D* directx);
void LogAdapterOutputs(D3D* directx, IDXGIAdapter* adapter);
void LogOutputDisplayModes(D3D* directx, IDXGIOutput* output, DXGI_FORMAT format);

void CreateCommandObjects(D3D* directx);
void CreateSwapChain(D3D* directx);
void CreateRtvAndDsvDescriptorHeaps(D3D* directx);


LRESULT CALLBACK
Message_Proc(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param); 

#endif
