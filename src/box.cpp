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

void Build_Desciptor_Heaps(ID3D12Device *device, Box *box) {}
void Build_Constant_Buffers(ID3D12Device *device, Box *box) {}
void Build_Root_Signature(ID3D12Device *device, Box *box) {}
void Build_Shaders_And_Input_Layout(ID3D12Device *device, Box *box) {}
void Build_Box_Geometry(ID3D12Device *device, Box *box) {}
void Build_PSO(ID3D12Device *device, Box *box) {}
