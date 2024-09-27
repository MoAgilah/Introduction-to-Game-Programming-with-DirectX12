#pragma once
//*********************************************************************
// This file is inspired by BoxApp.h by Frank Luna (C) 2015 All Rights Reserved.
// It incorporates my own coding style and naming conventions.
// It also includes exercise solutions divised by myself.
//
// Shows how to draw a box in Direct3D 12.
//
// Controls:
// Hold the left mouse button down and move the mouse to rotate.
// Hold the right mouse button down and move the mouse to zoom in and out.
// Key controls:
// Esc key - quit
// Ex.8 F3 - change between wireframe and solid fill mode
// Ex.9 F4 - change between disabling and enabling cull mode*
//		F5 - change between front and back face cull mode*
//		* visibility is easier to see in wireframe mode.
//*********************************************************************

#include "D3DApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

struct VPosData
{
	DirectX::XMFLOAT3 Pos;
};

struct VColorData
{
	DirectX::XMFLOAT4 Color;
};

struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 colour;
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	float gTime;
	DirectX::XMFLOAT4 PulseColour;
};

enum class Drawing
{
	BOX = 0,				// Base sample.
	BOXMULTIPLEINPUTSLOTS,	// Ex.2 use two vertex buffers and input slots to draw the box.
	PRIMITIVE,				// Ex.3 Draw - point list | line strip | line list | triangle strip | triangle list (change s_thisTopology appropiately).
	PYRAMID,				// Ex.4 Construct the vertex and index list of a pyramid.
	MULTIPLESHAPES			// Ex.7 Merge the vertices and indices of a box and pyramid into one large buffer. Then draw them one by one disjointed.
};

static const Drawing s_drawThis = Drawing::BOX;
static const D3D_PRIMITIVE_TOPOLOGY s_thisTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
/*
	Shader names:
	L"Shaders\\color.hlsl"								// Base shader
	L"Shaders\\animatedVertices.hlsl"					// Ex.6 modify the vertex shader to animate the vertices
	L"Shaders\\pulsatingColor.hlsl"						// Ex.16 modify the pixel shader to pulsate colours over time
	L"Shaders\\animatedVerticesPulsatingColor.hlsl"		// Combination of Ex.6 and Ex.16 solutions
*/
static const std::wstring s_shaderName = L"Shaders\\color.hlsl";

class BoxApp : public D3DApp
{
public:
	BoxApp();
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;

	bool Initialize() override;

private:
	void OnResize() override;
	void Update(const Timer& gt) override;
	void Draw(const Timer& gt) override;

	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;
	void OnKeyUp(WPARAM btnState) override;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();

	void BuildPrimitiveTopologyWithSingleInputSlots();
	void BuildBoxGeometryWithSingleInputSlots();
	void BuildPyramidGeometryWithSingleInputSlots();
	void BuildBoxAndPyramidGeometryWithSingleInputSlots();

	void BuildBoxGeometryWithMultipleInputSlots();

	void DrawPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);
	void DrawBoxGeometryTopologyWithSingleInputSlots();
	void DrawBoxGeometryTopologyWithMultipleInputSlots();
	void DrawPyramidGeometryTopology();
	void DrawBoxAndPyramidTopology();

	void BuildShadersAndInputLayoutWithSingleInputSlot();
	void BuildShadersAndInputLayoutWithMultipleInputSlots();
	void BuildPSO();

	void ChangeRasterizerState(bool& option);

private:

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> m_cbvHeap;

	std::vector<std::unique_ptr<UploadBuffer<ObjectConstants>>> m_objectCB;
	std::unique_ptr<MeshGeometry> m_meshGeo;

	Microsoft::WRL::ComPtr<ID3DBlob> m_vsByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> m_psByteCode;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO;

	DirectX::XMFLOAT4X4 m_world = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_view = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_proj = MathHelper::Identity4x4();

	bool m_wireFrameFillMode = false;
	bool m_disabledCulling = false;
	bool m_frontFaceCulling = false;
	float m_theta = 1.5f * DirectX::XM_PI;
	float m_phi = DirectX::XM_PIDIV4;
	float m_radius = 5.0f;
	POINT m_lastMousePos{};
};
