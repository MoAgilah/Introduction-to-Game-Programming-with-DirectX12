#pragma once

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "FrameResource.h"

struct RenderItem
{
    RenderItem() = default;

    // world matrix of the shape that describes the object's local space
    // relative to the world space, which define the position,
    // orientation, and scale of the object in the world.
    DirectX::XMFLOAT4X4 m_world;

    // Dirty flag indicating the object data has changed and we need to
    // update the constant buffer. Because we have an object cBuffer for
    // each FrameResouce, we have to apply the update to each
    // FrameResouce. Thus when we modify object data we should set
    // m_numFrameDirty = gNumFrameResources so that each FrameResource
    // gets the update
    int m_numFramesDirty = gNumFrameResources;

    // Index into GPU constant buffer corresponding to the ObjectCB
    // for this RenderItem
    UINT m_objCBIndex = -1;

    // Geometry associated with this RenderItem. Note that multiple
    // RenderItems can share the same geometry.
    MeshGeometry* m_geo = nullptr;

    // Primitive Topology
    D3D12_PRIMITIVE_TOPOLOGY m_primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters
    UINT m_indexCount = 0;
    UINT m_startIndexLocation = 0;
    int m_baseVertexLocation = 0;
};

class ShapesApp : public D3DApp
{
public:
    ShapesApp();
    ~ShapesApp();
    ShapesApp(const ShapesApp& rhs) = delete;
    ShapesApp& operator=(const ShapesApp& rhs) = delete;

    bool Initialize() override;

private:
    void OnResize() override;
    void Update(const Timer& gt) override;
    void Draw(const Timer& gt) override;

    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnMouseUp(WPARAM btnState, int x, int y) override;
    void OnMouseMove(WPARAM btnState, int x, int y) override;
    void OnKeyUp(WPARAM btnState) override;

    void OnKeyboardInput(const Timer& gt);
    void UpdateCamera(const Timer& gt);
    void UpdateObjectCBs(const Timer& gt);
    void UpdateMainPassCB(const Timer& gt);

    void BuildDescriptorHeaps();
    void BuildConstantBufferViews();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

    std::vector <std::unique_ptr<FrameResource>> m_frameResources;
    FrameResource* m_currFrameResource = nullptr;
    int m_currFrameResourceIndex = 0;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_geometries;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_shaders;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_PSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

    std::vector<std::unique_ptr<RenderItem>> m_allRItems;

    // Render items divided by PSO
    std::vector<RenderItem*> m_opaqueRItems;
    std::vector<RenderItem*> m_transparentRItems;

    PassConstants m_mainPassCB;

    UINT m_passCBVOffset = 0;

    bool m_isWireFrame = false;

    DirectX::XMFLOAT3 m_eyePos = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT4X4 m_view = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 m_proj = MathHelper::Identity4x4();

    float m_theta = 1.5f * DirectX::XM_PI;
    float m_phi = 0.2f * DirectX::XM_PI;
    float m_radius = 15.0f;

    POINT m_lastMousePos;
};