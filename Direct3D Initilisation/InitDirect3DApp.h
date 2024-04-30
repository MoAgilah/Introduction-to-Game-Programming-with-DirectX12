#pragma once
//***************************************************************************************
// This file is inspired by Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
// It incorporates my own coding style and naming conventions.
//
// Demonstrates the sample framework by initializing Direct3D, clearing
// the screen, and displaying frame stats.
//
//***************************************************************************************

#include "D3DApp.h"
#include <DirectXColors.h>

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp();

private:
	void Update(const Timer& gt) override;
	void Draw(const Timer& gt) override;

};