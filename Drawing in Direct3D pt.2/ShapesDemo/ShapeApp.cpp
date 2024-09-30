#include "ShapeApp.h"

const int gNumFrameResources = 3;

ShapesApp::ShapesApp()
	: D3DApp()
{
}

ShapesApp::~ShapesApp()
{
	if (m_device != nullptr)
		FlushCommandQueue();
}

bool ShapesApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands
	ThrowIfFailed(m_commandList->Reset(m_directCmdListAlloc.Get(), nullptr));

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildConstantBufferViews();
	BuildPSOs();

	// Execute the initialization commands.
	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void ShapesApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the
	// projection matrix.
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi,
		AspectRatio(), 1.0f, 1000.0f);

	XMStoreFloat4x4(&m_proj, P);
}

void ShapesApp::Update(const Timer& gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	m_currFrameResourceIndex = (m_currFrameResourceIndex + 1) % gNumFrameResources;
	m_currFrameResource = m_frameResources[m_currFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (m_currFrameResource->m_fence != 0 && m_dxgiFence->GetCompletedValue() < m_currFrameResource->m_fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_dxgiFence->SetEventOnCompletion(m_currFrameResource->m_fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(gt);
	UpdateMainPassCB(gt);
}

void ShapesApp::Draw(const Timer& gt)
{
	auto cmdListAlloc = m_currFrameResource->m_cmdListAlloc;

	// reuse the memory associated with command recording
	// we can only reset when associated command lists have
	// finished executing on the GPU
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been
	// added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	if (m_isWireFrame)
	{
		ThrowIfFailed(m_commandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque_wireframe"].Get()));
	}
	else
	{
		ThrowIfFailed(m_commandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));
	}

	m_commandList->RSSetViewports(1, &m_screenViewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// indicate a state transition on the resouce usage
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// clear the back buffer and depth buffer
	m_commandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	m_commandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	m_commandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	int passCbvIndex = m_passCBVOffset + m_currFrameResourceIndex;

	auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
	passCbvHandle.Offset(passCbvIndex,m_cbvSrvUavDescriptorSize);

	m_commandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

	DrawRenderItems(m_commandList.Get(), m_opaqueRItems);

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

	// Advance the fence value to mark commands up to this fence point.
	m_currFrameResource->m_fence = ++m_currentFence;

	// Add an instruction to the command queue to set a new fence point.
	// Because we are on the GPU timeline, the new fence point won’t be
	// set until the GPU finishes processing all the commands prior to this Signal().
	m_commandQueue->Signal(m_dxgiFence.Get(), m_currentFence);
}

void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;

	SetCapture(m_hMainWnd);
}

void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void ShapesApp::OnKeyUp(WPARAM btnState)
{
	D3DApp::OnKeyUp(btnState);
}

void ShapesApp::OnKeyboardInput(const Timer& gt)
{
	if (GetAsyncKeyState('1') & 0x8000)
		m_isWireFrame = true;
	else
		m_isWireFrame = false;
}

void ShapesApp::UpdateCamera(const Timer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	m_eyePos.x = m_radius * sinf(m_phi) * cosf(m_theta);
	m_eyePos.z = m_radius * sinf(m_phi) * sinf(m_theta);
	m_eyePos.y = m_radius * cosf(m_phi);

	// Build the view matrix.
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	DirectX::XMStoreFloat4x4(&m_view, view);
}

void ShapesApp::UpdateObjectCBs(const Timer& gt)
{
	auto currObjectCB = m_currFrameResource->m_objCB.get();

	for (auto& RItem : m_allRItems)
	{
		// Only update the cbuffer data if the constants
		// have changed.
		if (RItem->m_numFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&RItem->m_world);

			ObjectConstants objconstants;
			DirectX::XMStoreFloat4x4(&objconstants.World, DirectX::XMMatrixTranspose(world));

			currObjectCB->CopyData(RItem->m_objCBIndex, objconstants);

			// Next FrameResource needs to be updated too
			RItem->m_numFramesDirty--;
		}
	}
}

void ShapesApp::UpdateMainPassCB(const Timer& gt)
{
	DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&m_view);
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&m_proj);
	DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);
	DirectX::XMMATRIX invView =
		DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(view), view);
	DirectX::XMMATRIX invProj =
		DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(proj), proj);
	DirectX::XMMATRIX invViewProj =
		DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewProj),
			viewProj);
	DirectX::XMStoreFloat4x4(&m_mainPassCB.View,
		DirectX::XMMatrixTranspose(view));
	DirectX::XMStoreFloat4x4(&m_mainPassCB.InvView,
		DirectX::XMMatrixTranspose(invView));
	DirectX::XMStoreFloat4x4(&m_mainPassCB.Proj,
		DirectX::XMMatrixTranspose(proj));
	DirectX::XMStoreFloat4x4(&m_mainPassCB.InvProj,
		DirectX::XMMatrixTranspose(invProj));
	DirectX::XMStoreFloat4x4(&m_mainPassCB.ViewProj,
		DirectX::XMMatrixTranspose(viewProj));
	DirectX::XMStoreFloat4x4(&m_mainPassCB.InvViewProj,
		DirectX::XMMatrixTranspose(invViewProj));
	m_mainPassCB.EyePosW = m_eyePos;
	m_mainPassCB.RenderTargetSize =
		DirectX::XMFLOAT2((float)m_clientWidth, (float)m_clientHeight);
	m_mainPassCB.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f /
		m_clientWidth, 1.0f / m_clientHeight);
	m_mainPassCB.NearZ = 1.0f;
	m_mainPassCB.FarZ = 1000.0f;
	m_mainPassCB.TotalTime = gt.TotalTime();
	m_mainPassCB.DeltaTime = gt.DeltaTime();

	auto currPassCB = m_currFrameResource->m_passCB.get();
	currPassCB->CopyData(0, m_mainPassCB);
}

void ShapesApp::BuildDescriptorHeaps()
{
	UINT objCount = (UINT)m_opaqueRItems.size();

	//need a CBV descriptor for each object for each frame resource
	// + 1 for the perPass CBV for each frame resource.
	UINT numDescriptors = (objCount + 1) * gNumFrameResources;

	// save an offset to the start of the pass CBVs.
	// These are the last 3 descriptors
	m_passCBVOffset = objCount * gNumFrameResources;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&m_cbvHeap)));
};

void ShapesApp::BuildConstantBufferViews()
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof
	(ObjectConstants));

	UINT objCount = (UINT)m_opaqueRItems.size();

	// Need a CBV descriptor for each object for each
	//frame resource.
	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	{
		auto objectCB = m_frameResources[frameIndex]->m_objCB->Resource();
		for (UINT i = 0; i < objCount; ++i)
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbAddr = objectCB->GetGPUVirtualAddress();

			// Offset to the ith object constant buffer in the buffer.
			cbAddr += i * objCBByteSize;

			// Offset to the object CBV in the descriptor
			int heapIdx = frameIndex * objCount + i;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIdx, m_cbvSrvUavDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = cbAddr;
			cbvDesc.SizeInBytes = objCBByteSize;

			m_device->CreateConstantBufferView(&cbvDesc, handle);
		}
	}

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Last three descriptors are the pass CBVs for each frame resource.
	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	{
		auto passCB = m_frameResources[frameIndex]->m_passCB-> Resource();

		// Pass buffer only stores one cbuffer per frame resource.
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

		// Offset to the pass cbv in the descriptor heap.
		int heapIndex = m_passCBVOffset + frameIndex;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, m_cbvSrvUavDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = passCBByteSize;

		m_device->CreateConstantBufferView(&cbvDesc, handle);
	}
}

void ShapesApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	// Root parameter can be a table, rot descriptor or root constants
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	// Create rot CBVs
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
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
		serializedRootSig->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.GetAddressOf())));
}

void ShapesApp::BuildShadersAndInputLayout()
{
	m_shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
	m_shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");

	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void ShapesApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;

	GeometryGenerator::MeshData box =
		geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
	GeometryGenerator::MeshData grid =
		geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere =
		geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder =
		geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	/*
	We are concatenating all the geometry into one
	big vertex / index
	buffer. So define the regions in the buffer each
	submesh covers.
	*/

	// Cache the vertex offsets to each object in the
	// concatenated vertex buffer.
	UINT boxVtxOffset = 0;
	UINT gridVtxOffset = (UINT)box.m_vertices.size();
	UINT sphereVtxOffset = gridVtxOffset +
		(UINT)grid.m_vertices.size();
	UINT cylinderVtxOffset = sphereVtxOffset +
		(UINT)sphere.m_vertices.size();

	// Cache the starting index for each object in the
	// concatenated index buffer.
	UINT boxIdxOffset = 0;
	UINT gridIdxOffset = (UINT)box.m_indices32.size();
	UINT sphereIdxOffset = gridIdxOffset +
		(UINT)grid.m_indices32.size();
	UINT cylinderIdxOffset = sphereIdxOffset +
		(UINT)sphere.m_indices32.size();

	// Define the SubmeshGeometry that cover different
	// regions of the vertex/index buffers.
	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.m_indices32.size();
	boxSubmesh.StartIndexLocation = boxIdxOffset;
	boxSubmesh.BaseVertexLocation = boxVtxOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.m_indices32.size();
	gridSubmesh.StartIndexLocation = gridIdxOffset;
	gridSubmesh.BaseVertexLocation = gridVtxOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.m_indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIdxOffset;
	sphereSubmesh.BaseVertexLocation = sphereVtxOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.m_indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIdxOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVtxOffset;

	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.

	auto totalVertexCount =
		box.m_vertices.size() +
		grid.m_vertices.size() +
		sphere.m_vertices.size() +
		cylinder.m_vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.m_vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.m_vertices[i].m_position;
		vertices[k].Color = DirectX::XMFLOAT4(DirectX::Colors::DarkGreen);
	}

	for (size_t i = 0; i < grid.m_vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.m_vertices[i].m_position;
		vertices[k].Color = DirectX::XMFLOAT4(DirectX::Colors::ForestGreen);
	}

	for (size_t i = 0; i < sphere.m_vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.m_vertices[i].m_position;
		vertices[k].Color = DirectX::XMFLOAT4(DirectX::Colors::Crimson);
	}

	for (size_t i = 0; i < cylinder.m_vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.m_vertices[i].m_position;
		vertices[k].Color = DirectX::XMFLOAT4(DirectX::Colors::SteelBlue);
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(),
		std::begin(box.GetIndices16()),
		std::end(box.GetIndices16()));

	indices.insert(indices.end(),
		std::begin(grid.GetIndices16()),
		std::end(grid.GetIndices16()));

	indices.insert(indices.end(),
		std::begin(sphere.GetIndices16()),
		std::end(sphere.GetIndices16()));

	indices.insert(indices.end(),
		std::begin(cylinder.GetIndices16()),
		std::end(cylinder.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() *
		sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() *
		sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_device.Get(), m_commandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;

	m_geometries[geo->Name] = std::move(geo);
}

void ShapesApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };

	opaquePsoDesc.pRootSignature = m_rootSignature.Get();

	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_shaders["standardVS"]->GetBufferPointer()),
		m_shaders["standardVS"]->GetBufferSize()
	};

	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_shaders["opaquePS"]->GetBufferPointer()),
		m_shaders["opaquePS"]->GetBufferSize()
	};

	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = m_backBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = m_depthStencilFormat;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque"])));

	//
	// PSO for opaque wireframe objects.
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque_wireframe"])));
}

void ShapesApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		m_frameResources.push_back(std::make_unique<FrameResource>(
			m_device.Get(), 1, (UINT)m_allRItems.size()));
	}
}

void ShapesApp::BuildRenderItems()
{
	auto boxRItem = std::make_unique<RenderItem>();

	DirectX::XMStoreFloat4x4(&boxRItem->m_world, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) *
		DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f));

	boxRItem->m_objCBIndex = 0;
	boxRItem->m_geo = m_geometries["shapeGeo"].get();
	boxRItem->m_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRItem->m_indexCount = boxRItem->m_geo->DrawArgs["box"].IndexCount;
	boxRItem->m_startIndexLocation = boxRItem->m_geo-> DrawArgs["box"].
		StartIndexLocation;
	boxRItem->m_baseVertexLocation = boxRItem->m_geo-> DrawArgs["box"].
		BaseVertexLocation;
	m_allRItems.push_back(std::move(boxRItem));

	auto gridRItem = std::make_unique<RenderItem>();
	gridRItem->m_world = MathHelper::Identity4x4();
	gridRItem->m_objCBIndex = 1;
	gridRItem->m_geo = m_geometries["shapeGeo"].get();
	gridRItem->m_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRItem->m_indexCount = gridRItem->m_geo->DrawArgs["grid"].IndexCount;
	gridRItem->m_startIndexLocation = gridRItem->m_geo->DrawArgs["grid"].
		StartIndexLocation;
	gridRItem->m_baseVertexLocation = gridRItem->m_geo->DrawArgs["grid"].
		BaseVertexLocation;
	m_allRItems.push_back(std::move(gridRItem));

	UINT objCBIndex = 2;
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderItem>();
		DirectX::XMMATRIX leftCylWorld = DirectX::XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMStoreFloat4x4(&leftCylRitem->m_world, leftCylWorld);
		leftCylRitem->m_objCBIndex = objCBIndex++;
		leftCylRitem->m_geo = m_geometries["shapeGeo"].get();
		leftCylRitem->m_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->m_indexCount = leftCylRitem->m_geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->m_startIndexLocation = leftCylRitem->m_geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->m_baseVertexLocation = leftCylRitem->m_geo->DrawArgs["cylinder"].BaseVertexLocation;
		m_allRItems.push_back(std::move(leftCylRitem));

		auto rightCylRitem = std::make_unique<RenderItem>();
		DirectX::XMMATRIX rightCylWorld = DirectX::XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
		XMStoreFloat4x4(&rightCylRitem->m_world, rightCylWorld);
		rightCylRitem->m_objCBIndex = objCBIndex++;
		rightCylRitem->m_geo = m_geometries["shapeGeo"].get();
		rightCylRitem->m_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->m_indexCount = rightCylRitem->m_geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->m_startIndexLocation = rightCylRitem->m_geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->m_baseVertexLocation = rightCylRitem->m_geo->DrawArgs["cylinder"].BaseVertexLocation;
		m_allRItems.push_back(std::move(rightCylRitem));

		auto leftSphereRitem = std::make_unique<RenderItem>();
		DirectX::XMMATRIX leftSphereWorld = DirectX::XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMStoreFloat4x4(&leftSphereRitem->m_world, leftSphereWorld);
		leftSphereRitem->m_objCBIndex = objCBIndex++;
		leftSphereRitem->m_geo = m_geometries["shapeGeo"].get();
		leftSphereRitem->m_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->m_indexCount = leftSphereRitem->m_geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->m_startIndexLocation = leftSphereRitem->m_geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->m_baseVertexLocation = leftSphereRitem->m_geo->DrawArgs["sphere"].BaseVertexLocation;
		m_allRItems.push_back(std::move(leftSphereRitem));

		auto rightSphereRitem = std::make_unique<RenderItem>();
		DirectX::XMMATRIX rightSphereWorld = DirectX::XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);
		XMStoreFloat4x4(&rightSphereRitem->m_world, rightSphereWorld);
		rightSphereRitem->m_objCBIndex = objCBIndex++;
		rightSphereRitem->m_geo = m_geometries["shapeGeo"].get();
		rightSphereRitem->m_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->m_indexCount = rightSphereRitem->m_geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->m_startIndexLocation = rightSphereRitem->m_geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->m_baseVertexLocation = rightSphereRitem->m_geo->DrawArgs["sphere"].BaseVertexLocation;
		m_allRItems.push_back(std::move(rightSphereRitem));
	}

	// All the render items are opaque in this demo.
	for (auto& e : m_allRItems)
		m_opaqueRItems.push_back(e.get());
}

void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = m_currFrameResource->m_objCB->Resource();

	for (size_t i = 0; i < ritems.size(); i++)
	{
		auto ri = ritems[i];

		m_commandList->IASetVertexBuffers(0, 1, &ri->m_geo->VertexBufferView());
		m_commandList->IASetIndexBuffer(&ri->m_geo->IndexBufferView());
		m_commandList->IASetPrimitiveTopology(ri->m_primitiveType);

		// Offset to the CBV in the descriptor heap for this object and
		// for this frame resource.
		UINT cbvIndex = m_currFrameResourceIndex * (UINT)m_opaqueRItems.size() + ri->m_objCBIndex;

		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, m_cbvSrvUavDescriptorSize);

		cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);

		cmdList->DrawIndexedInstanced(ri->m_indexCount, 1, ri->m_startIndexLocation, ri->m_baseVertexLocation, 0);
	}
}