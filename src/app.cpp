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
					 L"Box",
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

bool Initialize_DirectX(D3D *directx)
{
  HRESULT result;
  
  result = CreateDXGIFactory1(IID_PPV_ARGS(&directx->dxgi_factory));
  if (FAILED(result))
    {
      return false; 
    }

  HRESULT hardware_result = D3D12CreateDevice(nullptr,
					      D3D_FEATURE_LEVEL_12_1,
					      IID_PPV_ARGS(&directx->d3d12_device));

  if (FAILED(hardware_result))
    {
      Microsoft::WRL::ComPtr<IDXGIAdapter> warp_adapter;
      result = directx->dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter));
      if (FAILED(result))
	{
	  return false; 
	}

      result = D3D12CreateDevice(warp_adapter.Get(),
				 D3D_FEATURE_LEVEL_12_1,
				 IID_PPV_ARGS(&directx->d3d12_device));

      if (FAILED(result))
	{
	  return false; 
	}
    }

  result = directx->d3d12_device->CreateFence(0,
					      D3D12_FENCE_FLAG_NONE,
					      IID_PPV_ARGS(&directx->fence));
  if (FAILED(result))
    {
      return false; 
    }

  directx->rtv_descriptor_size = directx->d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  directx->dsv_descriptor_size = directx->d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  directx->cbv_srv_uav_descriptor_size = directx->d3d12_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  CreateCommandObjects(directx);
  CreateSwapChain(directx);
  CreateRtvAndDsvDescriptorHeaps(directx);

  return true; 
}

void Release_DirectX(D3D *directx)
{
  if (directx->d3d12_device != nullptr)
    {
      FlushCommandQueue(directx);
    }
}

void Render(D3D* directx)
{
  HRESULT result;
  
  directx->direct_cmd_list_alloc->Reset();

  directx->command_list->Reset(directx->direct_cmd_list_alloc.Get(), nullptr);
  
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(directx->rtv_heap->GetCPUDescriptorHandleForHeapStart());
  for (UINT i = 0; i < directx->swap_chain_buffer_count; i++)
    {
      result = directx->swap_chain->GetBuffer(i, IID_PPV_ARGS(&directx->swap_chain_buffer[i]));
      directx->d3d12_device->CreateRenderTargetView(directx->swap_chain_buffer[i].Get(), nullptr, rtv_heap_handle);
      rtv_heap_handle.Offset(1, directx->rtv_descriptor_size); 
    }

  D3D12_RESOURCE_DESC depth_stencil_desc;
  ZeroMemory(&depth_stencil_desc, sizeof(depth_stencil_desc));
  depth_stencil_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  depth_stencil_desc.Alignment = 0;
  depth_stencil_desc.Width = directx->width;
  depth_stencil_desc.Height = directx->height;
  depth_stencil_desc.DepthOrArraySize = 1;
  depth_stencil_desc.MipLevels = 1;
  depth_stencil_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
  depth_stencil_desc.SampleDesc.Count = 1;
  depth_stencil_desc.SampleDesc.Quality = 0;
  depth_stencil_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  depth_stencil_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_CLEAR_VALUE opt_clear;
  opt_clear.Format = directx->depth_stencil_format;
  opt_clear.DepthStencil.Depth = 1.f;
  opt_clear.DepthStencil.Stencil = 0;

  result = directx->d3d12_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
							  D3D12_HEAP_FLAG_NONE,
							  &depth_stencil_desc,
							  D3D12_RESOURCE_STATE_COMMON,
							  &opt_clear,
							  IID_PPV_ARGS(directx->depth_stencil_buffer.GetAddressOf()));
  if (FAILED(result))
    {
      MessageBoxW(0, L"directx->d3d12_device->CreateCommittedResource Failed!", 0, MB_OK | MB_ICONERROR);
    }

  D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
  ZeroMemory(&dsv_desc, sizeof(dsv_desc)); 
  dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
  dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  dsv_desc.Format = directx->depth_stencil_format;
  dsv_desc.Texture2D.MipSlice = 0;

  directx->d3d12_device->CreateDepthStencilView(directx->depth_stencil_buffer.Get(),
						&dsv_desc,
						directx->dsv_heap->GetCPUDescriptorHandleForHeapStart());

  directx->command_list->ResourceBarrier(1,
					 &CD3DX12_RESOURCE_BARRIER::Transition(directx->depth_stencil_buffer.Get(),
									       D3D12_RESOURCE_STATE_COMMON,
									       D3D12_RESOURCE_STATE_DEPTH_WRITE));

  result = directx->command_list->Close();
  if (FAILED(result))
    {
      MessageBoxW(0, L"directx->command_list->Close() Failed!", 0, MB_OK | MB_ICONERROR);
    }

  ID3D12CommandList *cmds_lists[] = { directx->command_list.Get() };
  directx->command_queue->ExecuteCommandLists(_countof(cmds_lists), cmds_lists);

  FlushCommandQueue(directx);

  directx->screen_viewport.TopLeftX = 0;
  directx->screen_viewport.TopLeftY = 0;
  directx->screen_viewport.Width = static_cast<float>(directx->width);
  directx->screen_viewport.Height = static_cast<float>(directx->height);
  directx->screen_viewport.MinDepth = 0.f;
  directx->screen_viewport.MaxDepth = 1.f;

  directx->scissor_rect = { 0, 0, directx->width, directx->height };     
}

void CreateCommandObjects(D3D* directx)
{
  HRESULT result; 
  
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};

  ZeroMemory(&queue_desc, sizeof(queue_desc));
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  
  result = directx->d3d12_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&directx->command_queue));
  if (FAILED(result))
    {
      MessageBoxW(0, L"CreateCommandQueue Failed!", 0, MB_OK | MB_ICONERROR);
    }

  result = directx->d3d12_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
							 IID_PPV_ARGS(directx->direct_cmd_list_alloc.GetAddressOf()));
  if (FAILED(result))
    {
      MessageBoxW(0, L"CreateCommandAllocator Failed!", 0, MB_OK | MB_ICONERROR);
    }

  result = directx->d3d12_device->CreateCommandList(0,
						    D3D12_COMMAND_LIST_TYPE_DIRECT,
						    directx->direct_cmd_list_alloc.Get(),
						    nullptr,
						    IID_PPV_ARGS(directx->command_list.GetAddressOf()));
  if (FAILED(result))
    {
      MessageBoxW(0, L"CreateCommandList Failed!", 0, MB_OK | MB_ICONERROR);	    
    }
  
  directx->command_list->Close(); 
}

void CreateSwapChain(D3D* directx)
{
  HRESULT result;
  
  directx->swap_chain.Reset();

  DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
  swap_chain_desc.BufferDesc.Width = directx->width;
  swap_chain_desc.BufferDesc.Height = directx->height;
  swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
  swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
  swap_chain_desc.BufferDesc.Format = directx->back_buffer_format;
  swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  swap_chain_desc.SampleDesc.Count = 1;
  swap_chain_desc.SampleDesc.Quality = 0;
  swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_chain_desc.BufferCount = directx->swap_chain_buffer_count;
  swap_chain_desc.OutputWindow = directx->handle_window;
  swap_chain_desc.Windowed = true;
  swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  result = directx->dxgi_factory->CreateSwapChain(directx->command_queue.Get(),
						  &swap_chain_desc,
						  directx->swap_chain.GetAddressOf());
  if (FAILED(result))
    {
      MessageBoxW(0, L"CreateSwapChain Failed!", 0, MB_OK | MB_ICONERROR);
    }
}

void CreateRtvAndDsvDescriptorHeaps(D3D* directx)
{
  HRESULT result; 
  
  D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
  ZeroMemory(&rtv_heap_desc, sizeof(rtv_heap_desc));
  rtv_heap_desc.NumDescriptors	= directx->swap_chain_buffer_count;
  rtv_heap_desc.Type		= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtv_heap_desc.Flags		= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  rtv_heap_desc.NodeMask	= 0;

  result = directx->d3d12_device->CreateDescriptorHeap(&rtv_heap_desc,
						       IID_PPV_ARGS(directx->rtv_heap.GetAddressOf()));
  if (FAILED(result))
    {
      MessageBoxW(0, L"CreateDescriptorHeap RTV Failed!", 0, MB_OK | MB_ICONERROR);
    }

  D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc;
  ZeroMemory(&dsv_heap_desc, sizeof(dsv_heap_desc));
  dsv_heap_desc.NumDescriptors	= directx->swap_chain_buffer_count;
  dsv_heap_desc.Type		= D3D12_DESCRIPTOR_HEAP_TYPE_DSV;  
  dsv_heap_desc.Flags		= D3D12_DESCRIPTOR_HEAP_FLAG_NONE; 
  dsv_heap_desc.NodeMask	= 0;                               

  result = directx->d3d12_device->CreateDescriptorHeap(&dsv_heap_desc,
						       IID_PPV_ARGS(directx->dsv_heap.GetAddressOf()));
  if (FAILED(result))
    {
      MessageBoxW(0, L"CreateDescriptorHeap DSV Failed!", 0, MB_OK | MB_ICONERROR);
    }
}

void FlushCommandQueue(D3D* directx)
{
  HRESULT result; 
  
  directx->current_fence++;

  result = directx->command_queue->Signal(directx->fence.Get(), directx->current_fence);
  if (FAILED(result))
    {
      MessageBoxW(0, L"Signal Failed!", 0, MB_OK | MB_ICONERROR);
    }

  if (directx->fence->GetCompletedValue() < directx->current_fence)
    {
      HANDLE event_handle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

      result = directx->fence->SetEventOnCompletion(directx->current_fence, event_handle);
      if (FAILED(result))
	{
	  MessageBoxW(0, L"SetEventOnCompletion Failed!", 0, MB_OK | MB_ICONERROR);
	}
      
      WaitForSingleObject(event_handle, INFINITE);
      CloseHandle(event_handle);
    }
}


LRESULT CALLBACK
Message_Proc(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param)
{
  switch(message)
    {

    case WM_DESTROY:
      {
	PostQuitMessage(0);
	return 0;
      } break;
    };
  
  return DefWindowProcW(wnd, message, w_param, l_param);
} 
