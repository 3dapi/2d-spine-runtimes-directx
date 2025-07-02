#pragma once
#ifndef _SceneSpine_H_
#define _SceneSpine_H_

#include <map>
#include <vector>
#include <d3d11.h>
#include <DirectxMath.h>
#include <spine/spine.h>
#include "G2Util.h"

using namespace std;
using namespace DirectX;


struct VtxSequenceSpine
{
	enum ESPINE_ATTACHMENT_TYPE { ESPINE_MESH_ATTACH=1, ESPINE_MESH_REGION, };

	int				drawOrder	{};
	int				meshType	{};
	ID3D11Buffer*	bufPos		{};
	ID3D11Buffer*	bufTex		{};
	ID3D11Buffer*	bufDif		{};
	ID3D11Buffer*	bufIdx		{};
	size_t			countVtx	{};
	size_t			countIdx	{};
	D3D_PRIMITIVE_TOPOLOGY	primitve {D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP};
	~VtxSequenceSpine()	{
		G2::SAFE_RELEASE(bufPos);
		G2::SAFE_RELEASE(bufTex);
		G2::SAFE_RELEASE(bufDif);
		G2::SAFE_RELEASE(bufIdx);
		countVtx = {};
		countIdx = {};
	}
	int resourceBinding(int order, void* attachment, ESPINE_ATTACHMENT_TYPE attachmentType, size_t vertexCount, size_t indexCount =0);
	int draw(ID3D11DeviceContext* d3dContext, ID3D11ShaderResourceView* texSRV);
};

class SceneSpine : public spine::TextureLoader
{
protected:
	ID3D11Buffer*				m_cnstMVP			{};
	XMMATRIX					m_tmMVP				= XMMatrixIdentity();
	ID3D11RasterizerState*		m_stateRater		{};
	ID3D11BlendState*			m_stateBlend		{};
	ID3D11DepthStencilState*	m_stateDepthWrite	{};
	ID3D11VertexShader*			m_shaderVtx			{};
	ID3D11PixelShader*			m_shaderPxl			{};
	ID3D11InputLayout*			m_vtxLayout			{};

	std::vector<std::string>	m_spineAnimations	;
	map<int, VtxSequenceSpine*>	m_spineSequence		;
	ID3D11ShaderResourceView*	m_spineTexture		{};
	ID3D11SamplerState*			m_sampLinear		{};
	spine::Skeleton*			m_spineSkeleton		{};
	spine::AnimationState*		m_spineAniState		{};
	spine::SkeletonData*		m_spineSkeletonData	{};
	spine::Atlas*				m_spineAtlas		{};
public:
	SceneSpine();
	virtual ~SceneSpine();

	int		Init();
	int		Destroy();
	int		Update(float deltaTime);
	int		Render();
	void	SetMVP(const XMMATRIX& tmMVP);
protected:
	void	InitSpine();
	void	UpdateSpineSequence();
	void	UpdateSpineBuffer();
	void	SetupSpineSequence(int order, void* attachment, VtxSequenceSpine::ESPINE_ATTACHMENT_TYPE attachmentType);
	void	load(spine::AtlasPage& page,const spine::String& path) override;
	void	unload(void* texture) override;
};

#endif
