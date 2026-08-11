#include "app.h"

bool Initialize_Main_Window(D3D* directx)
{
  WNDCLASSW wc;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = Message_Proc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = directx->directx_instance;
  wc.hIcon = LoadIcon(0, IDI_APPLICATION);
  wc.hCursor = LoadCursor(0, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
  wc.lpszMenuName = 0;
  wc.lpszClassName = L"Box";

  if (!RegisterClassW(&wc))
    {
      MessageBoxW(0, L"RegisterClass Failed!", 0, MB_OK | MB_ICONERROR);
      return false; 
    }

  RECT r = { 0, 0, directx->width, directx->height };
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
  int width = r.right - r.left;
  int height = r.bottom - r.top;

  directx->handle_window = CreateWindowW(L"Box",
					 directx->window_caption,
					 WS_OVERLAPPEDWINDOW,
					 CW_USEDEFAULT, CW_USEDEFAULT,
					 width, height,
					 0, 0,
					 directx->directx_instance,
					 0);
  if (!directx->handle_window)
    {
      MessageBoxW(0, L"CreateWindow Failed!", 0, MB_OK | MB_ICONERROR);
      return false; 
    }

  ShowWindow(directx->handle_window, SW_SHOW);
  UpdateWindow(directx->handle_window);

  return true; 
}

int Run(D3D* directx)
{
  MSG message = {0};

  Reset(&directx->timer); 

  while(message.message != WM_QUIT)
    {
      if (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
	  TranslateMessage(&message);
	  DispatchMessage(&message); 
	}
      else
	{
	  Tick(&directx->timer);

	  if (!directx->paused)
	    {
	      CalculateFrameStats();
	      Update(directx, &directx->timer);
	      Draw(directx, &directx->timer);
	    }
	}
    }

  return (int)message.wParam;  
}

bool Initialize_DirectX(D3D *directx)
{
#if defined(DEBUG) || defined(DEBUG)
  {
    Microsoft::WRL::ComPtr<ID3D12Debug> debug_controller;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
    debug_controller->EnableDebugLayer(); 
  }

  ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&directx->dxgi_factory)));

  HRESULT hardware_result = D3D12CreateDevice(nullptr,
					      D3D_FEATURE_LEVEL_11_0 |
					      D3D_FEATURE_LEVEL_11_1 |
					      D3D_FEATURE_LEVEL_12_0 |
					      D3D_FEATURE_LEVEL_12_1,
					      IID_PPV_ARGS(&directx->d3d12_device));

  if (FAILED(hardware_result))
    {
      Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
      ThrowIfFailed(directx->dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)));

      ThrowIfFailedI(D3D12CreateDevice(warp_adapter.Get(),
				       D3D_FEATURE_LEVEL_11_0 |
				       D3D_FEATURE_LEVEL_11_1 |
				       D3D_FEATURE_LEVEL_12_0 |
				       D3D_FEATURE_LEVEL_12_1,
				       IID_PPV_ARGS(&directx->d3d12_device)));
    }

  ThrowIfFailed(directx->d3d12_device->CreateFence(0,
						   D3D12_FENCE_FLAG_NONE,
						   IID_PPV_ARGS(&directx->fence)));

  directx->rtv_descriptor_size = directx->d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  directx->dsv_descriptor_size = directx->d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  directx->cbv_srv_uav_descriptor_size = directx->d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_quality_levels;
  ZeroMemory(&ms_quality_levels, sizeof(ms_quality_levels));
  ms_quality_levels.Format = directx->back_buffer_format;
  ms_quality_levels.SampleCount = 4;
  ms_quality_levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
  ms_quality_levels.NumQualtiyLevels = 0;

  ThrowIfFailed(directx->d3d12_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
							   &ms_quality_levels,
							   sizeof(ms_quality_levels)));

  directx->m4x_msaa_quality = ms_quality_levels.NumQualityLevels;
  assert(directx->m4x_msaa_quality > 0 && "Unexpected MSAA quality level.");

  #ifdef _DEBUG
  LogAdapters(directx); 
  #endif

  CreateCommandObjects(directx);
  CreateSwapChain(directx);
  CreateRtvAndDsvDescriptorHeaps(directx);

  return true; 
}


void LogAdapters(D3D* directx)
{
  
}

void LogAdapterOutputs(D3D* directx, IDXGIAdapter* adapter);
void LogOutputDisplayModes(D3D* directx, IDXGIOutput* output, DXGI_FORMAT format);


void CreateCommandObjects(D3D* directx)
{
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};

  ZeroMemory(&queue_desc, sizeof(queue_desc));
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  
  ThrowIfFailed(directx->d3d12_device->CreateCommandQueue(&queue_desc), IID_PPV_ARGS(&directx->command_queue));

  ThrowIfFailed(directx->d3d12_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
							      IID_PPV_ARGS(directx->direct_cmd_list_alloc.GetAddressOf())));

  ThrowIfFailed(directx->d3d12_device->CreateCommandList(0,
							 D3D12_COMMAND_LIST_TYPE_DIRECT,
							 directx->direct_cmd_list_alloc.GetAddressOf(),
							 nullptr,
							 IID_PPV_ARGS(directx->command_list.GetAddressOf())));

  directx->command_list->Close(); 
}

void CreateSwapChain(D3D* directx)
{
  directx->swap_chain.Reset();

  DXGI_SWAP_CHAIN_DESC swap_chain_desc;
  swap_chain_desc.BufferDesc.Width = directx->width;
  swap_chain_desc.BufferDesc.Height = directx->height;
  swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
  swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
  swap_chain_desc.BufferDesc.Format = directx->back_buffer_format;
  swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_ORDER_UNSPECIFIED;
  swap_chain_desc.SampleDesc.Count = directx->m4x_msaa_quality ? 4 : 1;
  swap_chain_desc.SampleDesc.Quality = directx->m4x_msaa_quality ? (directx->m4x_msaa_quality - 1) : 0;
  swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_chain_desc.BufferCount = directx->swap_chain_buffer_count;
  swap_chain_desc.OutputWindow = directx->handle_window;
  swap_chain_desc.Windowed = true;
  swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  ThrowIfFailed(directx->dxgi_factory->CreateSwapChain(directx->command_queue.Get(),
						       &swap_chain_desc,
						       directx->swap_chain.GetAddressOf()));
}

void CreateRtvAndDsvDescriptorHeaps(D3D* directx)
{
  
}

LRESULT CALLBACK
Message_Proc(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param)
{
  switch(message)
    {
      
    };
} 
