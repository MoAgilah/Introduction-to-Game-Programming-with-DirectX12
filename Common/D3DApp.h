//***************************************************************************************
// This file is inspired by d3dApp.h by Frank Luna (C) 2015 All Rights Reserved.
// It incorporates my own coding style and naming conventions.
//***************************************************************************************
#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "Timer.h"

// link neccessary d3d12 libraries
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
public:
	static D3DApp* GetApp() { return m_app; }

	HINSTANCE AppInst() const { return m_hInst; };
	HWND MainWnd() const { return m_hMainWnd; }
	float AspectRatio() const { return static_cast<float>(m_clientWidth / m_clientHeight); };

	bool Get4xMsaaState() const { return m_4xMsaaState; }
	void Set4xMsaaState(bool value);

	int Run();

	virtual bool Initialize();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	D3DApp();
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

	virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize();
	virtual void Update(const Timer& t) = 0;
	virtual void Draw(const Timer& t) = 0;

	// convenience overrides for handling mouse input
	virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
	virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

	bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer() const
	{
		return m_swapChainBuffer[m_currBackBuffer].Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			m_currBackBuffer,
			m_rtvDescriptorSize);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const
	{
		return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	}

	void CalculateFrameStats();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) const;

protected:
	static D3DApp* m_app;

	HINSTANCE m_hInst = nullptr;
	HWND m_hMainWnd = nullptr;
	bool m_appPaused;
	bool m_minimized;
	bool m_maximized;
	bool m_resizing;
	bool m_fullScreenState;

	bool m_4xMsaaState;
	UINT m_4xMsaaQuality;

	Timer m_timer;

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;

	Microsoft::WRL::ComPtr<ID3D12Fence> m_dxgiFence;
	UINT64 m_currentFence;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_directCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

	static const int m_swapChainBufferCount = 2;
	int m_currBackBuffer;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, m_swapChainBufferCount> m_swapChainBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	D3D12_VIEWPORT m_screenViewport;
	D3D12_RECT m_scissorRect;

	UINT m_rtvDescriptorSize;
	UINT m_dsvDescriptorSize;
	UINT m_cbrSrvDescriptorSize;

	std::wstring m_mainWndCaption;
	D3D_DRIVER_TYPE m_d3dDriverType;
	DXGI_FORMAT m_backBufferFormat;
	DXGI_FORMAT m_depthStencilFormat;
	int m_clientWidth;
	int m_clientHeight;
};