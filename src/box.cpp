#include "box.h"

static UINT Calculate_Constant_Buffer_Byte_Size(UINT byte_size)
{
  // Constant buffers must be a multiple of the minimum hardware
  // allocation size (usually 256 bytes).  So round up to nearest
  // multiple of 256.  We do this by adding 255 and then masking off
  // the lower 2 bytes which store all bits < 256.
  // Example: Suppose byteSize = 300.
  // (300 + 255) & ~255
  // 555 & ~255
  // 0x022B & ~0x00ff
  // 0x022B & 0xff00
  // 0x0200
  // 512
  return (byte_size + 255) & ~255;
}

Buffer Upload_Buffer(ID3D12Device *device, UINT element_count, bool is_constant_buffer)  
{
  HRESULT result; 
  
  Buffer buffer;

  buffer.element_byte_size = sizeof(ObjectConstants); 

  if (is_constant_buffer)
    {
      buffer.element_byte_size = Calculate_Constant_Buffer_Byte_Size(sizeof(ObjectConstants));
    }

  result = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					   D3D12_HEAP_FLAG_NONE,
					   &CD3DX12_RESOURCE_DESC::Buffer(buffer.element_byte_size*element_count),
					   D3D12_RESOURCE_STATE_GENERIC_READ,
					   nullptr,
					   IID_PPV_ARGS(&buffer.resource_buffer));

  if (FAILED(result))
    {
      MessageBoxW(0, L"CreateCommittedResource Failed!", 0, MB_OK | MB_ICONERROR);
    }

  result = buffer.resource_buffer->Map(0, nullptr, reinterpret_cast<void**>(&buffer.mapped_data));
  if (FAILED(result))
    {
      MessageBoxW(0, L"buffer.resource_buffer->Map Failed!", 0, MB_OK | MB_ICONERROR);
    }

  return buffer; 
}

void Copy_Data(Buffer* buffer, int element_index, const ObjectConstants& data)
{
  memcpy(&buffer->mapped_data[element_index*buffer->element_byte_size], &data, sizeof(ObjectConstants)); 
}


static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const LPCWSTR& filename,
						      const D3D_SHADER_MACRO *defines,
						      const LPCSTR& entry_point,
						      const LPCSTR& target)
{
  UINT compile_flags = 0;
  HRESULT result = S_OK;

  Microsoft::WRL::ComPtr<ID3DBlob> byte_code = nullptr;
  Microsoft::WRL::ComPtr<ID3DBlob> errors;

  result = D3DCompileFromFile(filename, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry_point, target, compile_flags, 0, &byte_code, &errors); 
  if (errors != nullptr)
    {
      MessageBoxW(0, L"Shader has bug!", 0, MB_OK | MB_ICONERROR);
    }

  if (FAILED(result))
    {
      MessageBoxW(0, L"D3DCompileFromFile Failed!", 0, MB_OK | MB_ICONERROR);
    }

  return byte_code; 
}



D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view(MeshGeometry* mesh_geometry)
{
  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
  vertex_buffer_view.BufferLocation = mesh_geometry->vertex_buffer_gpu->GetGPUVirtualAddress();
  vertex_buffer_view.StrideInBytes = mesh_geometry->vertex_byte_stride;
  vertex_buffer_view.SizeInBytes = mesh_geometry->vertex_buffer_byte_size;

  return vertex_buffer_view; 
}

D3D12_INDEX_BUFFER_VIEW index_buffer_view(MeshGeometry* mesh_geometry)
{
  D3D12_INDEX_BUFFER_VIEW index_buffer_view;
  index_buffer_view.BufferLocation = mesh_geometry->index_buffer_gpu->GetGPUVirtualAddress();
  index_buffer_view.Format = mesh_geometry->index_format;
  index_buffer_view.SizeInBytes = mesh_geometry->index_buffer_byte_size;

  return index_buffer_view; 
}

void dispose_uploaders(MeshGeometry* mesh_geometry)
{
  mesh_geometry->vertex_buffer_uploader = nullptr;
  mesh_geometry->index_buffer_uploader = nullptr;
}


bool Initialize_Box(D3D* directx, Box *box)
{
  HRESULT result;

  result = directx->command_list->Reset(directx->direct_cmd_list_alloc.Get(), nullptr);
  if (FAILED(result))
    {
      MessageBoxW(0, L"directx->command_list->Reset Failed!", 0, MB_OK | MB_ICONERROR);
      return false;
    }
  
  Build_Desciptor_Heaps(directx->d3d12_device.Get(), box);
  Build_Constant_Buffers(directx->d3d12_device.Get(), box);
  Build_Root_Signature(directx->d3d12_device.Get(), box);
  Build_Shaders_And_Input_Layout(directx->d3d12_device.Get(), box);
  Build_Box_Geometry(directx->d3d12_device.Get(), box);
  Build_PSO(directx->d3d12_device.Get(), box); 
  
  result = directx->command_list->Close();
  if (FAILED(result))
    {
      MessageBoxW(0, L"directx->command_list->Close Failed!", 0, MB_OK | MB_ICONERROR);
      return false;
    }

  ID3D12CommandList *cmds_lists[] = { directx->command_list.Get() };
  directx->command_queue->ExecuteCommandLists(_countof(cmds_lists), cmds_lists); 

  FlushCommandQueue(directx);

  return true; 
}

void Release_Box(Box *box)
{
  if (box->object_cb)
    {
      delete box->object_cb; 
      box->object_cb = nullptr;
    }

  if (box->box_geo)
    {
      delete box->box_geo; 
      box->box_geo = nullptr;
    }
}

void Build_Desciptor_Heaps(ID3D12Device *device, Box *box)
{
  HRESULT result;
  
  D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc;
  cbv_heap_desc.NumDescriptors = 1; 
  cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  cbv_heap_desc.NodeMask = 0;

  result = device->CreateDescriptorHeap(&cbv_heap_desc, IID_PPV_ARGS(&box->cbv_heap));
  if (FAILED(result))
    {
      MessageBoxW(0, L"device->CreateDescriptorHeap Failed!", 0, MB_OK | MB_ICONERROR);
    }
}

void Build_Constant_Buffers(ID3D12Device *device, Box *box)
{
  box->object_cb = &Upload_Buffer(device, 1, true);

  UINT object_cb_byte_size = Calculate_Constant_Buffer_Byte_Size(sizeof(ObjectConstants));

  D3D12_GPU_VIRTUAL_ADDRESS cb_address = box->object_cb->resource_buffer->GetGPUVirtualAddress();

  int box_c_buf_index = 0;
  cb_address += box_c_buf_index * object_cb_byte_size;

  D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
  cbv_desc.BufferLocation = cb_address;
  cbv_desc.SizeInBytes = Calculate_Constant_Buffer_Byte_Size(sizeof(ObjectConstants));

  device->CreateConstantBufferView(&cbv_desc,
				   box->cbv_heap->GetCPUDescriptorHandleForHeapStart());
}

void Build_Root_Signature(ID3D12Device *device, Box *box)
{
  CD3DX12_ROOT_PARAMETER slot_root_parameter[1];

  CD3DX12_DESCRIPTOR_RANGE cbv_table;
  cbv_table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
  slot_root_parameter[0].InitAsDescriptorTable(1, &cbv_table);

  CD3DX12_ROOT_SIGNATURE_DESC root_sig_desc(1, slot_root_parameter, 0, nullptr,
					    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  Microsoft::WRL::ComPtr<ID3DBlob> serialized_root_sig = nullptr;
  Microsoft::WRL::ComPtr<ID3DBlob> error_blob = nullptr;

  HRESULT result = D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1,
					       serialized_root_sig.GetAddressOf(), error_blob.GetAddressOf());

  if (FAILED(result))
    {
      MessageBoxW(0, L"D3D12SerializeRootSignature Failed!", 0, MB_OK | MB_ICONERROR);
    }

  result = device->CreateRootSignature(0,
				       serialized_root_sig->GetBufferPointer(),
				       serialized_root_sig->GetBufferSize(),
				       IID_PPV_ARGS(&box->root_signature));
}

void Build_Shaders_And_Input_Layout(Box *box)
{
  HRESULT result = S_OK;

  box->vertex_shader_byte_code = CompileShader(L"color.hlsl", nullptr, "ColorVertexShader", "vs_5_0");
  box->pixel_shader_byte_code = CompileShader(L"color.hlsl", nullptr, "ColorPixelShader", "ps_5_0");
  
  box->input_layout =
    {
      {
	"POSITION",
	0,
	DXGI_FORMAT_R32G32B32_FLOAT,
	0, 0,
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
      },
      {
	"COLOR",
	0,
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	0, D3D12_APPEND_ALIGNED_ELEMENT,
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
      }
    };
}

void Build_Box_Geometry(ID3D12Device *device, Box *box)
{
  std::array<Vertex, 8> vertices =
    {
      Vertex({ float3(-1.f, -1.f, -1.f), float4(DirectX::Colors::White  ) }),
      Vertex({ float3(-1.f,  1.f, -1.f), float4(DirectX::Colors::Black  ) }),
      Vertex({ float3( 1.f,  1.f, -1.f), float4(DirectX::Colors::Red    ) }),
      Vertex({ float3( 1.f, -1.f, -1.f), float4(DirectX::Colors::Green  ) }),
      Vertex({ float3(-1.f, -1.f,  1.f), float4(DirectX::Colors::Blue   ) }),
      Vertex({ float3(-1.f,  1.f,  1.f), float4(DirectX::Colors::Yellow ) }),
      Vertex({ float3( 1.f,  1.f,  1.f), float4(DirectX::Colors::Cyan   ) }),
      Vertex({ float3( 1.f, -1.f,  1.f), float4(DirectX::Colors::Magenta) }),
    };

  std::array<std::uint16_t, 36> indices =
    {
      0, 1, 2,
      0, 2, 3,

      4, 6, 5,
      4, 7, 6,

      4, 5, 1,
      4, 1, 0,

      3, 2, 6,
      3, 6, 7,

      1, 5, 6,
      1, 6, 2,

      4, 0, 3,
      4, 3, 7
    };

  const UINT vertex_buffer_byte_size = (UINT)vertices.size() * sizeof(Vertex);
  const UINT index_buffer_byte_size = (UINT)indices.size() * sizeof(std::uint16_t);

  box->box_geo->name = L"box_geo";

  HRESULT result = D3DCreateBlob(vertex_buffer_byte_size,
				 &box->box_geo->vertex_buffer_cpu);
  if (FAILED(result))
    {
      MessageBoxW(0, L"D3DCreateBlob Vertex Failed!", 0, MB_OK | MB_ICONERROR);
    }
  CopyMemory(box->box_geo->vertex_buffer_cpu->GetBufferPointer(), vertices.data(), vertex_buffer_byte_size);
  
  result = D3DCreateBlob(index_buffer_byte_size,
			 &box->box_geo->index_buffer_cpu);
  if (FAILED(result))
    {
      MessageBoxW(0, L"D3DCreateBlob Vertex Failed!", 0, MB_OK | MB_ICONERROR);
    }
  CopyMemory(box->box_geo->index_buffer_cpu->GetBufferPointer(), indices.data(), index_buffer_byte_size);

  
  // Add CreateDefaultBuffer
  
  
}

void Build_PSO(ID3D12Device *device, Box *box)
{
  
}



void Update(Box *box)
{
  float x = box->radius*std::sinf(box->phi)*std::cosf(box->theta);
  float z = box->radius*std::sinf(box->phi)*std::sinf(box->theta);
  float y = box->radius*std::cosf(box->phi);

  vec4 pos = XMVectorSet(x, y, z, 1.f);
  vec4 target = XMVectorZero();
  vec4 up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

  matrix view = XMMatrixLookAtLH(pos, target, up);
  XMStoreFloat4x4(&box->view, view);

  matrix world = XMLoadFloat4x4(&box->world);
  matrix projection = XMLoadFloat4x4(&box->projection);
  matrix world_view_projection = world*view*projection;

  ObjectConstants object_constants;
  XMStoreFloat4x4(&object_constants.WorldViewProj, XMMatrixTranspose(world_view_projection));
  Copy_Data(box->object_cb, 0, object_constants);
}

void Draw(D3D *directx, Box *box)
{
  directx->direct_cmd_list_alloc->Reset();

  directx->command_list->Reset(directx->direct_cmd_list_alloc.Get(), box->pso.Get());

  directx->command_list->RSSetViewports(1, &directx->screen_viewport);
  directx->command_list->RSSetScissorRects(1, &directx->scissor_rect);

  directx->command_list->ResourceBarrier(1,
					 &CD3DX12_RESOURCE_BARRIER::Transition(directx->swap_chain_buffer[directx->current_back_buffer].Get(),
									       D3D12_RESOURCE_STATE_PRESENT,
									       D3D12_RESOURCE_STATE_RENDER_TARGET));

  directx->command_list->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(directx->rtv_heap->GetCPUDescriptorHandleForHeapStart(),
									   directx->current_back_buffer,
									   directx->rtv_descriptor_size),
					       DirectX::Colors::MediumPurple,
					       0, nullptr);
  directx->command_list->ClearDepthStencilView(directx->dsv_heap->GetCPUDescriptorHandleForHeapStart(),
					       D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
					       1.f, 0, 0, nullptr);

  directx->command_list->OMSetRenderTargets(1,
					    &CD3DX12_CPU_DESCRIPTOR_HANDLE(directx->rtv_heap->GetCPUDescriptorHandleForHeapStart(),
									   directx->current_back_buffer,
									   directx->rtv_descriptor_size),
					    true,
					    &directx->dsv_heap->GetCPUDescriptorHandleForHeapStart());

  ID3D12DescriptorHeap *descriptor_heaps[] = { box->cbv_heap.Get() };
  directx->command_list->SetDescriptorHeaps(_countof(descriptor_heaps), descriptor_heaps);

  directx->command_list->SetGraphicsRootSignature(box->root_signature.Get());

  directx->command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view(box->box_geo));
  directx->command_list->IASetIndexBuffer(&index_buffer_view(box->box_geo));
  directx->command_list->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  directx->command_list->SetGraphicsRootDescriptorTable(0, box->cbv_heap->GetGPUDescriptorHandleForHeapStart());

  directx->command_list->DrawIndexedInstanced(box->box_geo->draw_args[L"box"].index_count,
					      1, 0, 0, 0);
  
  directx->command_list->ResourceBarrier(1,
					 &CD3DX12_RESOURCE_BARRIER::Transition(directx->swap_chain_buffer[directx->current_back_buffer].Get(),
									       D3D12_RESOURCE_STATE_RENDER_TARGET,
									       D3D12_RESOURCE_STATE_PRESENT));

  HRESULT result;

  result = directx->command_list->Close();
  if (FAILED(result))
    {
      MessageBoxW(0, L"directx->command_list->Close() Failed!", 0, MB_OK | MB_ICONERROR);
    }

  ID3D12CommandList *cmd_lists[] = { directx->command_list.Get() };
  directx->command_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);

  directx->swap_chain->Present(0, 0);
  directx->current_back_buffer = (directx->current_back_buffer + 1) % directx->swap_chain_buffer_count;

  FlushCommandQueue(directx); 
}

int Run(D3D *directx, Box *box)
{
  MSG message = {0};

  while(message.message != WM_QUIT)
    {
      if (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
	{
	  TranslateMessage(&message);
	  DispatchMessageW(&message); 
	}
      else
	{
	  Update(box);
	  Draw(directx, box);
	}
    }

  return (int)message.wParam;  
}
