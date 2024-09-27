#pragma once

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "FrameResource.h"

static const int g_numFrameResources = 3;

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
    // m_numFrameDirty = g_numFrameResources so that each FrameResource
    // gets the update
    int m_numFramesDirty = g_numFrameResources;

    // Index into GPU constant buffer corresponding to the ObjectCB
    // for this RenderItem
    UINT m_objCBIndex = -1;

    // Geometry associated with this RenderItem. Note that multiple
    // RenderItems can share the same geometry.
    MeshGeometry* m_geo = nullptr;

    // Primitive Toplogy
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
    ShapesApp(const ShapesApp& rhs) = delete;
    ShapesApp& operator=(const ShapesApp& rhs) = delete;
    ~ShapesApp() = default;

    virtual bool Initialize() override;

private:
    virtual void OnResize() override;
    virtual void Update(const Timer& gt) override;
    virtual void Draw(const Timer& gt) override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

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

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    std::vector<std::unique_ptr<RenderItem>> m_allRItems;

    // Render items divided by PSO
    std::vector<RenderItem*> m_opaqueRItems;
    std::vector<RenderItem*> m_transparentRItems;

    PassConstants m_mainPassCB;

    UINT m_passCBVOffset = 0;

    bool m_isWireFrame = false;

    DirectX::XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f * DirectX::XM_PI;
    float mPhi = 0.2f * DirectX::XM_PI;
    float mRadius = 15.0f;

    POINT mLastMousePos;
};