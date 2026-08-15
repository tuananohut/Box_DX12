#ifndef BOX_H
#define BOX_H

#include <assert.h>

#include <DirectXCollision.h>
#include <windows.h>
#include <wrl.h>
#include <unordered_map>
#include <vector>
#include <memory>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXColors.h>
#include <d3d12.h>
#include <cmath>
#include <array>

#include "d3dx12.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdi32.lib")

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


struct Buffer
{
  Microsoft::WRL::ComPtr<ID3D12Resource> resource_buffer;
  BYTE* mapped_data = nullptr;

  UINT element_byte_size = 0;
  bool is_constant_buffer = false; 
};

static UINT Calculate_Constant_Buffer_Byte_Size(UINT byte_size); 

Buffer Upload_Buffer(ID3D12Device *device,
		     UINT element_count,
		     bool is_constant_buffer);

void Copy_Data(Buffer* buffer, int element_index, const ObjectConstants& data);


static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const LPCWSTR& filename,
						      const D3D_SHADER_MACRO *defines,
						      const LPCSTR& entry_point,
						      const LPCSTR& target);

static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device *device,
								 ID3D12GraphicsCommandList *cmd_list,
								 const void *init_data,
								 UINT64 byte_size,
								 Microsoft::WRL::ComPtr<ID3D12Resource>& resource_buffer);


struct SubmeshGeometry
{
  UINT index_count = 0;
  UINT start_index_location = 0;
  INT base_vertex_location = 0;
  
  DirectX::BoundingBox bounds; 
};

struct MeshGeometry
{
  LPCWSTR name;

  Microsoft::WRL::ComPtr<ID3DBlob> vertex_buffer_cpu = nullptr;
  Microsoft::WRL::ComPtr<ID3DBlob> index_buffer_cpu = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer_gpu = nullptr;
  Microsoft::WRL::ComPtr<ID3D12Resource> index_buffer_gpu = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer_uploader = nullptr;
  Microsoft::WRL::ComPtr<ID3D12Resource> index_buffer_uploader = nullptr; 

  UINT vertex_byte_stride = 0;
  UINT vertex_buffer_byte_size = 0;
  DXGI_FORMAT index_format = DXGI_FORMAT_R16_UINT;
  UINT index_buffer_byte_size = 0;

  std::unordered_map<LPCWSTR, SubmeshGeometry> draw_args; 
};

D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view(MeshGeometry* mesh_geometry);
  D3D12_INDEX_BUFFER_VIEW index_buffer_view(MeshGeometry* mesh_geometry);
void dispose_uploaders(MeshGeometry* mesh_geometry); 



struct Box
{
  Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature = nullptr;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbv_heap = nullptr; 

  Buffer *object_cb = nullptr; 

  MeshGeometry *box_geo = nullptr;

  Microsoft::WRL::ComPtr<ID3DBlob> vertex_shader_byte_code = nullptr;
  Microsoft::WRL::ComPtr<ID3DBlob> pixel_shader_byte_code = nullptr;

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout; 
  
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pso = nullptr; 

  float4x4 world = Identity4x4();
  float4x4 view = Identity4x4();
  float4x4 projection = Identity4x4();

  float theta = 1.5f * XM_PI;
  float phi = XM_PIDIV4;
  float radius = 5.f;
};

bool Initialize_Box(D3D *directx, Box *box);
void Release_Box(Box *box); 

void Build_Desciptor_Heaps(ID3D12Device *device, Box *box);
void Build_Constant_Buffers(ID3D12Device *device, Box *box);
void Build_Root_Signature(ID3D12Device *device, Box *box);
void Build_Shaders_And_Input_Layout(Box *box);
void Build_Box_Geometry(ID3D12Device *device,
			ID3D12GraphicsCommandList* command_list, 
			Box *box);
void Build_PSO(ID3D12Device *device,
	       DXGI_FORMAT back_buffer_format,
	       DXGI_FORMAT depth_stencil_format,
	       Box *box); 

void Update(Box *box);
void Draw(D3D* directx, Box *box);
int Run(D3D *directx, Box *box);

#endif
