#pragma once
//*********************************************************************
// This file is inspired by BoxApp.h by Frank Luna (C) 2015 All Rights Reserved.
// It incorporates my own coding style and naming conventions.
//
// Shows how to draw a box in Direct3D 12.
//
// Controls:
// Hold the left mouse button down and move the mouse to rotate.
// Hold the right mouse button down and move the mouse to zoom in and
// out.
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
};

enum class Drawing
{
	BOX = 0, PRIMITIVE, PYRAMID
};

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

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();

	void BuildPrimitiveTopology();
	void BuildBoxGeometryWithSingleInputSlots();
	void BuildBoxGeometryWithMultipleInputSlots();

	void DrawPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);
	void DrawBoxGeometryTopology();

	void BuildShadersAndInputLayoutWithSingleInputSlot();
	void BuildShadersAndInputLayoutWithMultipleInputSlots();
	void BuildPSO();

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap;

	std::unique_ptr<UploadBuffer<ObjectConstants>> m_objectCB;
	std::unique_ptr<MeshGeometry> m_meshGeo;

	Microsoft::WRL::ComPtr<ID3DBlob> m_vsByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> m_psByteCode;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO;

	DirectX::XMFLOAT4X4 m_world = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_view = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_proj = MathHelper::Identity4x4();

	Drawing m_drawThis = Drawing::BOX;
	bool m_multipleInputSlots = false;
	float m_theta = 1.5f * DirectX::XM_PI;
	float m_phi = DirectX::XM_PIDIV4;
	float m_radius = 5.0f;
	POINT m_lastMousePos{};
};
