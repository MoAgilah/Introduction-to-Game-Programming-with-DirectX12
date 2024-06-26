//*********************************************************************
// This file is inspired by BoxApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
// It incorporates my own coding style and naming conventions.
//
// Shows how to draw a box in Direct3D 12.
//*********************************************************************
#include "BoxApp.h"

BoxApp::BoxApp()
	: D3DApp()
{
	m_cbvHeap.resize(2);
	m_objectCB.resize(2);
}

bool BoxApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands
	ThrowIfFailed(m_commandList->Reset(m_directCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();

	switch (s_drawThis)
	{
	case Drawing::BOX:
		BuildShadersAndInputLayoutWithSingleInputSlot();
		BuildBoxGeometryWithSingleInputSlots();
		break;
	case Drawing::BOXMULTIPLEINPUTSLOTS:
		BuildShadersAndInputLayoutWithMultipleInputSlots();
		BuildBoxGeometryWithMultipleInputSlots();
		break;
	case Drawing::PRIMITIVE:
		BuildShadersAndInputLayoutWithSingleInputSlot();
		BuildPrimitiveTopologyWithSingleInputSlots();
		break;
	case Drawing::PYRAMID:
		BuildShadersAndInputLayoutWithSingleInputSlot();
		BuildPyramidGeometryWithSingleInputSlots();
		break;
	case Drawing::MULTIPLESHAPES:
		BuildShadersAndInputLayoutWithSingleInputSlot();
		BuildBoxAndPyramidGeometryWithSingleInputSlots();
		break;
	default:
		break;
	}

	BuildPSO();

	// Execute the initialization commands.
	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the
	// projection matrix.
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi,
		AspectRatio(), 1.0f, 1000.0f);

	XMStoreFloat4x4(&m_proj, P);
}

void BoxApp::Update(const Timer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = m_radius * sinf(m_phi) * cosf(m_theta);
	float z = m_radius * sinf(m_phi) * sinf(m_theta);
	float y = m_radius * cosf(m_phi);

	// Build the view matrix.
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	DirectX::XMStoreFloat4x4(&m_view, view);

	DirectX::XMMATRIX world= DirectX::XMLoadFloat4x4(&m_world);
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&m_proj);
	DirectX::XMMATRIX worldViewProj = world * view * proj;

	// Update the constant buffer with the latest worldViewProj matrix.
	ObjectConstants objConstants;

	if (s_drawThis == Drawing::MULTIPLESHAPES)
	{
		DirectX::XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProj * DirectX::XMMatrixScaling(0.5, 0.5, 0.5) * DirectX::XMMatrixTranslation(-0.5, 0, 0)));
		objConstants.gTime = gt.TotalTime();
		DirectX::XMStoreFloat4(&objConstants.PulseColour, DirectX::Colors::Red);
		m_objectCB[0]->CopyData(0, objConstants);

		DirectX::XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProj * DirectX::XMMatrixScaling(0.5, 0.5, 0.5) * DirectX::XMMatrixTranslation(0.5, 0, 0)));
		objConstants.gTime = gt.TotalTime();
		DirectX::XMStoreFloat4(&objConstants.PulseColour, DirectX::Colors::Red);
		m_objectCB[1]->CopyData(0, objConstants);
	}
	else
	{
		DirectX::XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProj));
		objConstants.gTime = gt.TotalTime();
		DirectX::XMStoreFloat4(&objConstants.PulseColour, DirectX::Colors::Red);
		m_objectCB[0]->CopyData(0, objConstants);
	}
}

void BoxApp::Draw(const Timer& gt)
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished
	// execution on the GPU.
	ThrowIfFailed(m_directCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the
	// command queue via ExecuteCommandList. Reusing the command
	// list reuses memory.
	ThrowIfFailed(m_commandList->Reset(m_directCmdListAlloc.Get(), m_PSO.Get()));

	m_commandList->RSSetViewports(1, &m_screenViewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate a state transition on the resource usage.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// clear the back buffer and depth buffer
	m_commandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	m_commandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	m_commandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap[0].Get() };
	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	switch (s_drawThis)
	{
	case Drawing::BOX:
		DrawBoxGeometryTopologyWithSingleInputSlots();
		break;
	case Drawing::BOXMULTIPLEINPUTSLOTS:
		DrawBoxGeometryTopologyWithSingleInputSlots();
		break;
	case Drawing::PRIMITIVE:
		DrawPrimitiveTopology(s_thisTopology);
		break;
	case Drawing::PYRAMID:
		DrawPyramidGeometryTopology();
		break;
	case Drawing::MULTIPLESHAPES:
		DrawBoxAndPyramidTopology();
		break;
	default:
		break;
	}

	// Indicate a state transition on the resouce usage.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(m_commandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(m_swapChain->Present(0, 0));
	m_currBackBuffer = (m_currBackBuffer + 1) % m_swapChainBufferCount;

	// Wait until frame commands are complete. This waiting is
	// inefficient and is done for simplicity. Later we will show how to
	// organize our rendering code so we do not have to wait per frame.
	FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;

	SetCapture(m_hMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x -
			m_lastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y -
			m_lastMousePos.y));

		// Update angles based on input to orbit camera around box.
		m_theta += dx;
		m_phi += dy;

		// Restrict the angle m_phi.
		m_phi = MathHelper::Clamp(m_phi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f * static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - m_lastMousePos.y);

		// Update the camera radius based on input.
		m_radius += dx - dy;

		// Restrict the radius.
		m_radius = MathHelper::Clamp(m_radius, 3.0f, 15.0f);
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void BoxApp::OnKeyUp(WPARAM btnState)
{
	if ((int)btnState == VK_F3)
	{
		ChangeRasterizerState(m_wireFrameFillMode);
	}
	else if ((int)btnState == VK_F4)
	{
		ChangeRasterizerState(m_disabledCulling);
	}
	else if ((int)btnState == VK_F5)
	{
		ChangeRasterizerState(m_frontFaceCulling);
	}
	else
	{
		D3DApp::OnKeyUp(btnState);
	}
}

void BoxApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&m_cbvHeap[0])));

	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&m_cbvHeap[1])));
}

void BoxApp::BuildConstantBuffers()
{
	m_objectCB[0] = std::make_unique<UploadBuffer<ObjectConstants>>(m_device.Get(), 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_objectCB[0]->Resource()->GetGPUVirtualAddress();

	// Offset to the ith object constant bufer in the buffer
	// Here our i == 0
	int boxCbufIndex = 0;
	cbAddress += boxCbufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap[0]->GetCPUDescriptorHandleForHeapStart());

	m_objectCB[1] = std::make_unique<UploadBuffer<ObjectConstants>>(m_device.Get(), 1, true);

	objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	cbAddress = m_objectCB[1]->Resource()->GetGPUVirtualAddress();

	// Offset to the ith object constant bufer in the buffer
	// Here our i == 0
	cbAddress += boxCbufIndex * objCBByteSize;

	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap[1]->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
	// Shader programs typically require resources as input (constant
	// buffers, textures, samplers). The root signature defines the
	// resources the shader programs expect. If we think of the shader
	// programs as a function, and the input resources as function
	// parameters, then the root signature can be thought of as defining
	// the function signature.

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// Create a single descriptor table of CBVs
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a
	// descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(hr);

	ThrowIfFailed(m_device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void BoxApp::BuildPrimitiveTopologyWithSingleInputSlots()
{
	std::array<Vertex, 8> vertices =
	{
		Vertex({ DirectX::XMFLOAT3(-1.5f, -0.5f, 0.5f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ DirectX::XMFLOAT3(-1.0f, 0.5f, 0.5f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ DirectX::XMFLOAT3(-0.5f, -0.2f, 0.5f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ DirectX::XMFLOAT3(0.2f, 0.2f, 0.5f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ DirectX::XMFLOAT3(0.5f, -0.3f, 0.5f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ DirectX::XMFLOAT3(0.75f, 0.2f, 0.5f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ DirectX::XMFLOAT3(1.25f, -0.1f, 0.5f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ DirectX::XMFLOAT3(1.5f, 0.75f, 0.5f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
	};

	std::array<std::uint16_t, 8> indices =
	{
		0,1,2,3,4,5,6,7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	m_meshGeo = std::make_unique<MeshGeometry>();
	m_meshGeo->Name = "genGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_meshGeo->VertexBufferCPU));
	CopyMemory(m_meshGeo->VertexBufferCPU->GetBufferPointer(),
		vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_meshGeo->IndexBufferCPU));
	CopyMemory(m_meshGeo->IndexBufferCPU->GetBufferPointer(),
		indices.data(), ibByteSize);

	m_meshGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		vertices.data(), vbByteSize,
		m_meshGeo->VertexBufferUploader);

	m_meshGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		indices.data(), ibByteSize,
		m_meshGeo->IndexBufferUploader);

	m_meshGeo->VertexByteStride = sizeof(Vertex);
	m_meshGeo->VertexBufferByteSize = vbByteSize;
	m_meshGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_meshGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();

	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	m_meshGeo->DrawArgs["gen"] = submesh;
}

void BoxApp::BuildBoxGeometryWithSingleInputSlots()
{
	std::array<Vertex, 8> vertices =
	{
		Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) }),
		Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) }),
		Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
		Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) }),
		Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) }),
		Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) }),
		Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta)})
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,
		// back face
		4, 6, 5,
		4, 7, 6,
		// left face
		4, 5, 1,
		4, 1, 0,
		// right face
		3, 2, 6,
		3, 6, 7,
		// top face
		1, 5, 6,
		1, 6, 2,
		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	m_meshGeo = std::make_unique<MeshGeometry>();
	m_meshGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_meshGeo->VertexBufferCPU));
	CopyMemory(m_meshGeo->VertexBufferCPU->GetBufferPointer(),
		vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_meshGeo->IndexBufferCPU));
	CopyMemory(m_meshGeo->IndexBufferCPU->GetBufferPointer(),
		indices.data(), ibByteSize);

	m_meshGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		vertices.data(), vbByteSize,
		m_meshGeo->VertexBufferUploader);

	m_meshGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		indices.data(), ibByteSize,
		m_meshGeo->IndexBufferUploader);

	m_meshGeo->VertexByteStride = sizeof(Vertex);
	m_meshGeo->VertexBufferByteSize = vbByteSize;
	m_meshGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_meshGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();

	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	m_meshGeo->DrawArgs["box"] = submesh;
}

void BoxApp::BuildPyramidGeometryWithSingleInputSlots()
{
	std::array<Vertex, 5> vertices =
	{
		// Square base of the pyramid
		Vertex({ DirectX::XMFLOAT3(-1.0, -1.0, 1.0), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
		Vertex({ DirectX::XMFLOAT3(1.0f, -1.0, 1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
		Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
		Vertex({ DirectX::XMFLOAT3(1.0f, -1.0, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),

		// The tip of the pyramid
		Vertex({ DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) }),
	};

	std::array<std::uint16_t, 18> indices =
	{
		0, 1, 4,
		1, 3, 4,
		3, 2, 4,
		2, 0, 4,
		0, 2, 1,
		1, 2, 3,
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	m_meshGeo = std::make_unique<MeshGeometry>();
	m_meshGeo->Name = "pyramidGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_meshGeo->VertexBufferCPU));
	CopyMemory(m_meshGeo->VertexBufferCPU->GetBufferPointer(),
		vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_meshGeo->IndexBufferCPU));
	CopyMemory(m_meshGeo->IndexBufferCPU->GetBufferPointer(),
		indices.data(), ibByteSize);

	m_meshGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		vertices.data(), vbByteSize,
		m_meshGeo->VertexBufferUploader);

	m_meshGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		indices.data(), ibByteSize,
		m_meshGeo->IndexBufferUploader);

	m_meshGeo->VertexByteStride = sizeof(Vertex);
	m_meshGeo->VertexBufferByteSize = vbByteSize;
	m_meshGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_meshGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();

	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	m_meshGeo->DrawArgs["pyramid"] = submesh;
}

void BoxApp::BuildBoxAndPyramidGeometryWithSingleInputSlots()
{
	std::array<Vertex, 9> vertices =
	{
		Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White)}),
		Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black)}),
		Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) }),
		Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green)}),
		Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue)}),
		Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow)}),
		Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan)}),
		Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta)}),
		Vertex({ DirectX::XMFLOAT3(+0.0f, +1.0f, +0.0f), DirectX::XMFLOAT4(DirectX::Colors::Red)}),
	};

	std::array<std::uint16_t, 54> indices =
	{
		//box indices
		// front face
		0, 1, 2,
		0, 2, 3,
		// back face
		4, 6, 5,
		4, 7, 6,
		// left face
		4, 5, 1,
		4, 1, 0,
		// right face
		3, 2, 6,
		3, 6, 7,
		// top face
		1, 5, 6,
		1, 6, 2,
		// bottom face
		4, 0, 3,
		4, 3, 7,
		//pyramid indices
		4, 7, 8,
		7, 3, 8,
		3, 0, 8,
		0, 4, 8,
		4, 0, 7,
		7, 0, 3,
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	m_meshGeo = std::make_unique<MeshGeometry>();
	m_meshGeo->Name = "multipleGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_meshGeo->VertexBufferCPU));
	CopyMemory(m_meshGeo->VertexBufferCPU->GetBufferPointer(),
		vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_meshGeo->IndexBufferCPU));
	CopyMemory(m_meshGeo->IndexBufferCPU->GetBufferPointer(),
		indices.data(), ibByteSize);

	m_meshGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		vertices.data(), vbByteSize,
		m_meshGeo->VertexBufferUploader);

	m_meshGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		indices.data(), ibByteSize,
		m_meshGeo->IndexBufferUploader);

	m_meshGeo->VertexByteStride = sizeof(Vertex);
	m_meshGeo->VertexBufferByteSize = vbByteSize;
	m_meshGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_meshGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = 36;

	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	m_meshGeo->DrawArgs["box"] = submesh;

	submesh.IndexCount = 18;

	submesh.StartIndexLocation = 36;
	submesh.BaseVertexLocation = 0;

	m_meshGeo->DrawArgs["pyramid"] = submesh;
}

void BoxApp::BuildBoxGeometryWithMultipleInputSlots()
{
	std::array<VPosData, 8> vertices =
	{
		VPosData({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f) }),
		VPosData({ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f) }),
		VPosData({ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f) }),
		VPosData({ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f) }),
		VPosData({ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f) }),
		VPosData({ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f) }),
		VPosData({ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f) }),
		VPosData({ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f) }),
	};

	std::array<VColorData, 8> colors =
	{
		VColorData({ DirectX::XMFLOAT4(DirectX::Colors::White) }),
		VColorData({ DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		VColorData({ DirectX::XMFLOAT4(DirectX::Colors::Red) }),
		VColorData({ DirectX::XMFLOAT4(DirectX::Colors::Green) }),
		VColorData({ DirectX::XMFLOAT4(DirectX::Colors::Blue) }),
		VColorData({ DirectX::XMFLOAT4(DirectX::Colors::Yellow) }),
		VColorData({ DirectX::XMFLOAT4(DirectX::Colors::Cyan) }),
		VColorData({ DirectX::XMFLOAT4(DirectX::Colors::Magenta)})
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,
		// back face
		4, 6, 5,
		4, 7, 6,
		// left face
		4, 5, 1,
		4, 1, 0,
		// right face
		3, 2, 6,
		3, 6, 7,
		// top face
		1, 5, 6,
		1, 6, 2,
		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(VPosData);
	const UINT cbByteSize = (UINT)colors.size() * sizeof(VColorData);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	m_meshGeo = std::make_unique<MeshGeometry>();
	m_meshGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_meshGeo->VertexBufferCPU));
	CopyMemory(m_meshGeo->VertexBufferCPU->GetBufferPointer(),
		vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(cbByteSize, &m_meshGeo->ColorBufferCPU));
	CopyMemory(m_meshGeo->ColorBufferCPU->GetBufferPointer(),
		colors.data(), cbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_meshGeo->IndexBufferCPU));
	CopyMemory(m_meshGeo->IndexBufferCPU->GetBufferPointer(),
		indices.data(), ibByteSize);

	m_meshGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		vertices.data(), vbByteSize,
		m_meshGeo->VertexBufferUploader);

	m_meshGeo->ColorBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		colors.data(), cbByteSize,
		m_meshGeo->ColorBufferUploader);

	m_meshGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_device.Get(), m_commandList.Get(),
		indices.data(), ibByteSize,
		m_meshGeo->IndexBufferUploader);

	m_meshGeo->VertexByteStride = sizeof(VPosData);
	m_meshGeo->VertexBufferByteSize = vbByteSize;
	m_meshGeo->ColorByteStride = sizeof(VColorData);
	m_meshGeo->ColorBufferByteSize = cbByteSize;
	m_meshGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_meshGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();

	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	m_meshGeo->DrawArgs["box"] = submesh;
}

void BoxApp::DrawPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
{
	m_commandList->IASetVertexBuffers(0, 1, &m_meshGeo->VertexBufferView());
	m_commandList->IASetIndexBuffer(&m_meshGeo->IndexBufferView());
	m_commandList->IASetPrimitiveTopology(topology);

	m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap[0]->GetGPUDescriptorHandleForHeapStart());

	m_commandList->DrawIndexedInstanced(m_meshGeo->DrawArgs["gen"].IndexCount, 1, 0, 0, 0);
}

void BoxApp::DrawBoxGeometryTopologyWithSingleInputSlots()
{
	m_commandList->IASetVertexBuffers(0, 1, &m_meshGeo->VertexBufferView());

	m_commandList->IASetIndexBuffer(&m_meshGeo->IndexBufferView());
	m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap[0]->GetGPUDescriptorHandleForHeapStart());

	m_commandList->DrawIndexedInstanced(m_meshGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);
}

void BoxApp::DrawBoxGeometryTopologyWithMultipleInputSlots()
{
	m_commandList->IASetVertexBuffers(0, 1, &m_meshGeo->VertexBufferView());
	m_commandList->IASetVertexBuffers(1, 1, &m_meshGeo->ColorBufferView());

	m_commandList->IASetIndexBuffer(&m_meshGeo->IndexBufferView());
	m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap[0]->GetGPUDescriptorHandleForHeapStart());

	m_commandList->DrawIndexedInstanced(m_meshGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);
}

void BoxApp::DrawPyramidGeometryTopology()
{
	m_commandList->IASetVertexBuffers(0, 1, &m_meshGeo->VertexBufferView());

	m_commandList->IASetIndexBuffer(&m_meshGeo->IndexBufferView());
	m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap[0]->GetGPUDescriptorHandleForHeapStart());

	m_commandList->DrawIndexedInstanced(m_meshGeo->DrawArgs["pyramid"].IndexCount, 1, 0, 0, 0);
}

void BoxApp::DrawBoxAndPyramidTopology()
{
	m_commandList->IASetVertexBuffers(0, 1, &m_meshGeo->VertexBufferView());

	m_commandList->IASetIndexBuffer(&m_meshGeo->IndexBufferView());
	m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap[0]->GetGPUDescriptorHandleForHeapStart());

	m_commandList->DrawIndexedInstanced(m_meshGeo->DrawArgs["box"].IndexCount, 1, m_meshGeo->DrawArgs["box"].StartIndexLocation, 0, 0);

	m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap[1]->GetGPUDescriptorHandleForHeapStart());

	m_commandList->DrawIndexedInstanced(m_meshGeo->DrawArgs["pyramid"].IndexCount, 1, m_meshGeo->DrawArgs["pyramid"].StartIndexLocation, 0, 0);
}

void BoxApp::BuildShadersAndInputLayoutWithSingleInputSlot()
{
	m_vsByteCode = d3dUtil::CompileShader(s_shaderName, nullptr,
		"VS", "vs_5_0");
	m_psByteCode = d3dUtil::CompileShader(s_shaderName, nullptr,
		"PS", "ps_5_0");

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void BoxApp::BuildShadersAndInputLayoutWithMultipleInputSlots()
{
	m_vsByteCode = d3dUtil::CompileShader(s_shaderName, nullptr,
		"VS", "vs_5_0");
	m_psByteCode = d3dUtil::CompileShader(s_shaderName, nullptr,
		"PS", "ps_5_0");

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void BoxApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };

	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS =
	{
	reinterpret_cast<BYTE*>(m_vsByteCode->GetBufferPointer()),
	m_vsByteCode->GetBufferSize()
	};

	psoDesc.PS =
	{
	reinterpret_cast<BYTE*>(m_psByteCode->GetBufferPointer()),
	m_psByteCode->GetBufferSize()
	};

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	if (m_disabledCulling)
	{
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	}
	else
	{
		if (m_frontFaceCulling)
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		else
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	}

	psoDesc.RasterizerState.FillMode = m_wireFrameFillMode ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_backBufferFormat;
	psoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = m_depthStencilFormat;

	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));
}

void BoxApp::ChangeRasterizerState(bool& option)
{
	if (option)
		option = false;
	else
		option = true;

	BuildPSO();
}
