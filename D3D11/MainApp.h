#pragma once
#ifndef _MainApp_H_
#define _MainApp_H_

#include <memory>
#include <d3d11.h>
#include <DirectxMath.h>
#include "GameTimer.h"
#include "G2Util.h"
#include "SceneSpine.h"

using namespace DirectX;

class MainApp
{
protected:
	ID3D11Buffer*				m_cnstWorld			{};
	ID3D11Buffer*				m_cnstView			{};
	ID3D11Buffer*				m_cnstProj			{};
	XMMATRIX					m_mtView			{};
	XMMATRIX					m_mtProj			{};
	XMMATRIX					m_mtWorld			{};
	GameTimer					mTimer				;

	unique_ptr<SceneSpine>		m_spine				{};
public:
	MainApp();
	virtual ~MainApp();
	virtual int Init();
	virtual int Destroy();
	virtual int Update();
	virtual int Render();
};

#endif
