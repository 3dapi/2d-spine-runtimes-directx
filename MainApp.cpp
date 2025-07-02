#pragma warning(disable: 4081 4267)

#include <any>
#include <Windows.h>
#include <d3d11.h>
#include <DirectxColors.h>
#include <WICTextureLoader.h>
#include "G2Base.h"
#include "MainApp.h"

using namespace DirectX;

MainApp::MainApp()
{
}

MainApp::~MainApp()
{
	Destroy();
}

int MainApp::Init()
{
	auto d3dDevice  = std::any_cast<ID3D11Device*>(IG2GraphicsD3D::getInstance()->GetDevice());
	auto d3dContext = std::any_cast<ID3D11DeviceContext*>(IG2GraphicsD3D::getInstance()->GetContext());
	HRESULT hr = S_OK;
	
	auto spineInst = make_unique<SceneSpine>();
	if(spineInst)
	{
		if(SUCCEEDED(spineInst->Init()))
		{
			m_spine = std::move(spineInst);
		}
	}

	if(FAILED(hr))
		return hr;

	// 3. Create the constant buffer
	// 3.1 world
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(m_mtWorld);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = d3dDevice->CreateBuffer(&bd, {}, &m_cnstWorld);
	if (FAILED(hr))
		return hr;

	// 3.2 view
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(m_mtView);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = d3dDevice->CreateBuffer(&bd, {}, &m_cnstView);
	if (FAILED(hr))
		return hr;

	// 3.3 projection matrtix
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(m_mtProj);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = d3dDevice->CreateBuffer(&bd, {}, &m_cnstProj);
	if (FAILED(hr))
		return hr;

	// 4. setup the world, view, projection matrix
	// View, Projection Matrix
	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, -1000.0f, 0.0f );
	XMVECTOR At = XMVectorSet ( 0.0f, 0.0f,   0.0f, 0.0f );
	XMVECTOR Up = XMVectorSet ( 0.0f, 1.0f,   0.0f, 0.0f );
	m_mtView = XMMatrixLookAtLH(Eye, At, Up);
	// Initialize the projection matrix
	auto screeSize = std::any_cast<::SIZE*>(IG2GraphicsD3D::getInstance()->GetAttrib(ATTRIB_CMD::ATTRIB_SCREEN_SIZE));
	m_mtProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, screeSize->cx / (FLOAT)screeSize->cy, 1.0f, 5000.0f);
	// 4.2 Initialize the world matrix
	m_mtWorld = XMMatrixIdentity();

	mTimer.Reset();
	return S_OK;
}

int MainApp::Destroy()
{
	G2::SAFE_RELEASE(m_cnstWorld);
	G2::SAFE_RELEASE(m_cnstView	);
	G2::SAFE_RELEASE(m_cnstProj	);
	m_spine = nullptr;
	return S_OK;
}

int MainApp::Update()
{
	mTimer.Tick();
	auto t = mTimer.DeltaTime();
	if(m_spine)
	{
		XMMATRIX mvp = m_mtWorld * m_mtView * m_mtProj;
		m_spine->SetMVP(mvp);
		m_spine->Update(t);
	}
	return S_OK;
}


int MainApp::Render()
{
	auto d3dDevice  = std::any_cast<ID3D11Device*>(IG2GraphicsD3D::getInstance()->GetDevice());
	auto d3dContext = std::any_cast<ID3D11DeviceContext*>(IG2GraphicsD3D::getInstance()->GetContext());
	if(m_spine)
		m_spine->Render();

	return S_OK;
}
