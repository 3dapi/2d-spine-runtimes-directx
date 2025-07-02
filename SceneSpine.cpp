#pragma warning(disable: 4081 4267)

#include <any>
#include <Windows.h>
#include <d3d11.h>
#include <DirectxColors.h>
#include <WICTextureLoader.h>
#include "G2Base.h"
#include "SceneSpine.h"

using namespace DirectX;
using namespace spine;

namespace spine {
	SpineExtension *getDefaultExtension()
	{
		static SpineExtension* _default_spineExtension = new DefaultSpineExtension;
		return _default_spineExtension;
	}
}

int VtxSequenceSpine::resourceBinding(int order, void* attachment, ESPINE_ATTACHMENT_TYPE attachmentType, size_t vertexCount, size_t indexCount /*=0*/)
{
	HRESULT hr = S_OK;
	auto d3dDevice  = std::any_cast<ID3D11Device*>(IG2GraphicsD3D::getInstance()->GetDevice());
	if(!d3dDevice)
		return E_FAIL;

	if(0 >= vertexCount)
		return E_FAIL;

	this->drawOrder= order;
	this->meshType = attachmentType;
	this->countVtx = vertexCount;
	this->countIdx = indexCount;
	// vertex buffer
	{
		// position
		D3D11_BUFFER_DESC ibDesc ={};
		ibDesc.Usage = D3D11_USAGE_DYNAMIC;
		ibDesc.ByteWidth = (UINT)vertexCount * sizeof(XMFLOAT2);
		ibDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		ibDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		hr = d3dDevice->CreateBuffer(&ibDesc,nullptr,&bufPos);
		if(FAILED(hr))
			return hr;
		// texture coord
		ibDesc.ByteWidth = (UINT)vertexCount * sizeof(XMFLOAT2);
		hr = d3dDevice->CreateBuffer(&ibDesc,nullptr,&bufTex);
		if(FAILED(hr))
			return hr;
		// diffuse
		ibDesc.ByteWidth = (UINT)vertexCount * sizeof(uint32_t);
		hr = d3dDevice->CreateBuffer(&ibDesc,nullptr,&bufDif);
		if(FAILED(hr))
			return hr;

		primitve = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	}
	// index buffer
	if(indexCount)
	{
		D3D11_BUFFER_DESC ibDesc ={};
		ibDesc.Usage = D3D11_USAGE_DYNAMIC;
		ibDesc.ByteWidth = (UINT)indexCount * sizeof(uint16_t);
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		hr = d3dDevice->CreateBuffer(&ibDesc,nullptr,&bufIdx);
		if(FAILED(hr))
			return hr;

		primitve = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	return S_OK;
}

int VtxSequenceSpine::draw(ID3D11DeviceContext* d3dContext, ID3D11ShaderResourceView* texSRV)
{
	ID3D11Buffer* buffers[] ={ bufPos, bufDif, bufTex};
	UINT strides[] ={sizeof(XMFLOAT2), sizeof(uint32_t), sizeof(XMFLOAT2)};
	UINT offsets[] ={0,0,0};
	d3dContext->IASetVertexBuffers(0, 3, buffers, strides, offsets);
	d3dContext->IASetPrimitiveTopology(primitve);
	d3dContext->PSSetShaderResources(0,1,&texSRV);
	if(countIdx)
	{
		d3dContext->IASetIndexBuffer(bufIdx, DXGI_FORMAT_R16_UINT, 0);
		d3dContext->DrawIndexed(countIdx, 0, 0);
	}
	else
	{
		d3dContext->Draw(countVtx, 0);
	}
	return 0;
}

SceneSpine::SceneSpine()
{
}

SceneSpine::~SceneSpine()
{
	Destroy();
}

int SceneSpine::Init()
{
	HRESULT hr = S_OK;
	auto d3dDevice  = std::any_cast<ID3D11Device*>(IG2GraphicsD3D::getInstance()->GetDevice());
	auto d3dContext = std::any_cast<ID3D11DeviceContext*>(IG2GraphicsD3D::getInstance()->GetContext());

	// 1.1 create vertex shader
	ID3DBlob* pBlob{};
	hr = G2::DXCompileShaderFromFile("assets/shader/spine.fx", "main_vtx", "vs_4_0", &pBlob);
	if (FAILED(hr))
	{
		MessageBox({}, "The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
		return hr;
	}
	// 1.2 Create the vertex shader
	hr = d3dDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), {}, &m_shaderVtx);
	if (FAILED(hr))
		return hr;

	// 1.3 create vertexLayout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT   , 0, 0 , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR"   , 0, DXGI_FORMAT_R8G8B8A8_UNORM , 1, 0 , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT   , 2, 0 , D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	hr = d3dDevice->CreateInputLayout(layout, numElements, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &m_vtxLayout);
	G2::SAFE_RELEASE(pBlob);
	if (FAILED(hr))
		return hr;

	// 2.1 Compile the pixel shader
	hr = G2::DXCompileShaderFromFile("assets/shader/spine.fx", "main_pxl", "ps_4_0", &pBlob);
	if (FAILED(hr))
	{
		MessageBox({}, "Failed ComplThe FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", "Error", MB_OK);
		return hr;
	}
	// 2.2 Create the pixel shader
	hr = d3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), {}, &m_shaderPxl);
	G2::SAFE_RELEASE(pBlob);
	if (FAILED(hr))
		return hr;

	
	// 3. create texture sampler state
	{
		D3D11_SAMPLER_DESC sampDesc = {};
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = d3dDevice->CreateSamplerState(&sampDesc, &m_sampLinear);
		if (FAILED(hr))
			return hr;
	}

	// 4. render state
	{
		D3D11_RASTERIZER_DESC rasterDesc ={};
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.CullMode = D3D11_CULL_NONE;		// 또는 D3D11_CULL_NONE, D3D11_CULL_FRONT
		rasterDesc.FrontCounterClockwise = TRUE;	// CCW 면이 앞면 (보통 OpenGL 스타일)
		hr = d3dDevice->CreateRasterizerState(&rasterDesc,&m_stateRater);
		if(SUCCEEDED(hr)) {
			d3dContext->RSSetState(m_stateRater);
		}
	}
	{
		D3D11_BLEND_DESC blendDesc ={};
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = d3dDevice->CreateBlendState(&blendDesc,&m_stateBlend);
		if(FAILED(hr))
		{
			return hr;
		}
	}
	{
		D3D11_DEPTH_STENCIL_DESC  dsDesc ={};
		dsDesc.DepthEnable = FALSE;								// no depth test
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;	// no z write
		dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;				// always pas
		dsDesc.StencilEnable = FALSE;
		hr = d3dDevice->CreateDepthStencilState(&dsDesc,&m_stateDepthWrite);
		if(FAILED(hr))
		{
			return hr;
		}
	}

	// 5. Create the constant buffer
	// 5.1 tm mvp
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(m_tmMVP);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = d3dDevice->CreateBuffer(&bd, {}, &m_cnstMVP);
	if (FAILED(hr))
		return hr;

	InitSpine();

	return S_OK;
}

int SceneSpine::Destroy()
{
	G2::SAFE_RELEASE(m_shaderVtx);
	G2::SAFE_RELEASE(m_shaderPxl);
	G2::SAFE_RELEASE(m_vtxLayout);
	G2::SAFE_RELEASE(m_cnstMVP	);

	G2::SAFE_RELEASE(m_spineTexture);
	G2::SAFE_RELEASE(m_sampLinear);
	return S_OK;
}

int SceneSpine::Update(float deltaTime)
{
	// Update and apply the animation state to the skeleton
	m_spineAniState->update(deltaTime);
	m_spineAniState->apply(*m_spineSkeleton);

	// Update the skeleton time (used for physics)
	m_spineSkeleton->update(deltaTime);

	// Calculate the new pose
	m_spineSkeleton->updateWorldTransform(spine::Physics_Update);

	// update spine sequence
	UpdateSpineSequence();
	// update vertex
	UpdateSpineBuffer();
	return S_OK;
}

int SceneSpine::Render()
{
	auto d3dDevice  = std::any_cast<ID3D11Device*>(IG2GraphicsD3D::getInstance()->GetDevice());
	auto d3dContext = std::any_cast<ID3D11DeviceContext*>(IG2GraphicsD3D::getInstance()->GetContext());

	// 0. render state
	d3dContext->RSSetState(m_stateRater);
	float blendFactor[4] ={0,0,0,0};
	UINT sampleMask = 0xffffffff;
	d3dContext->OMSetBlendState(m_stateBlend,blendFactor,sampleMask);
	d3dContext->OMSetDepthStencilState(m_stateDepthWrite,0);

	// 1. Update constant value
	d3dContext->UpdateSubresource(m_cnstMVP, 0, {}, &m_tmMVP, 0, 0);

	// 2. set the constant buffer
	d3dContext->VSSetConstantBuffers(0, 1, &m_cnstMVP);

	// 3. set vertex shader
	d3dContext->VSSetShader(m_shaderVtx, {}, 0);
	// 4. set the input layout
	d3dContext->IASetInputLayout(m_vtxLayout);
	// 5. set the pixel shader
	d3dContext->PSSetShader(m_shaderPxl, {}, 0);

	// 6. set the sampler state
	d3dContext->PSSetSamplers(0,1,&m_sampLinear);

	for(auto [drawOrder, curSqc] : m_spineSequence)
	{
		curSqc->draw(d3dContext, m_spineTexture);
	}
	
	return S_OK;
}

void SceneSpine::SetMVP(const XMMATRIX& tmMVP)
{
	m_tmMVP = tmMVP;
}


void SceneSpine::InitSpine()
{
	Bone::setYDown(false);

	//m_spineAtlas = new Atlas("assets/spine/spineboy-pma/spineboy-pma.atlas", this);
	//SkeletonBinary binary(m_spineAtlas);
	//m_spineSkeletonData = binary.readSkeletonDataFile("assets/spine/spineboy-pma/spineboy-pro.skel");

	m_spineAtlas = new Atlas("assets/spine/raptor/raptor-pma.atlas",this);
	SkeletonJson binary(m_spineAtlas);
	m_spineSkeletonData = binary.readSkeletonDataFile("assets/spine/raptor/raptor-pro.json");

	m_spineSkeleton = new Skeleton(m_spineSkeletonData);
	m_spineSkeleton->setPosition(0.0F, -300.0F);
	m_spineSkeleton->setScaleX(0.6f);
	m_spineSkeleton->setScaleY(0.6f);

	spine::SkeletonData* skelData = m_spineSkeleton->getData();
	auto& animations = skelData->getAnimations();
	// animation name
	for(int i = 0; i < animations.size(); ++i) {
		spine::Animation* anim = animations[i];
		std::string animName = anim->getName().buffer();
		m_spineAnimations.push_back(animName);
	}

	AnimationStateData animationStateData(m_spineSkeletonData);
	animationStateData.setDefaultMix(0.2f);
	m_spineAniState = new AnimationState(&animationStateData);
	m_spineAniState->setAnimation(0,"gun-holster", false);
	m_spineAniState->addAnimation(0,"roar",false, 0.8F);
	m_spineAniState->addAnimation(0,"walk",true, 2.1F);
}

void SceneSpine::UpdateSpineSequence()
{
	auto drawOrder = m_spineSkeleton->getDrawOrder();
	for(size_t i = 0; i < drawOrder.size(); ++i)
	{
		spine::Slot* slot = drawOrder[i];
		spine::Attachment* attachment = slot->getAttachment();
		if(!attachment)
			continue;
		if(attachment->getRTTI().isExactly(spine::MeshAttachment::rtti))
		{
			SetupSpineSequence(i, attachment, VtxSequenceSpine::ESPINE_MESH_ATTACH);
		}
		else if(attachment->getRTTI().isExactly(spine::RegionAttachment::rtti))
		{
			SetupSpineSequence(i, attachment, VtxSequenceSpine::ESPINE_MESH_REGION);
		}
	}
}

void SceneSpine::UpdateSpineBuffer()
{
	auto d3dContext = std::any_cast<ID3D11DeviceContext*>(IG2GraphicsD3D::getInstance()->GetContext());
	for(auto [drawOrder, curSqc] : m_spineSequence)
	{
		spine::Slot* slot = m_spineSkeleton->getDrawOrder().operator[](drawOrder);
		spine::Attachment* attachment = slot->getAttachment();
		if(curSqc->meshType == VtxSequenceSpine::ESPINE_ATTACHMENT_TYPE::ESPINE_MESH_ATTACH)
		{
			auto* attm = static_cast<spine::MeshAttachment*>(attachment);
			auto* mesh = static_cast<spine::MeshAttachment*>(attachment);

			// 색상
			spine::Color c = slot->getColor();
			c.a *= slot->getColor().a;
			c.r *= slot->getColor().r;
			c.g *= slot->getColor().g;
			c.b *= slot->getColor().b;

			spine::Color mColor = mesh->getColor();
			c.a *= mColor.a; c.r *= mColor.r; c.g *= mColor.g; c.b *= mColor.b;

			// 색상
			uint32_t rgba =
				(uint32_t(c.a * 255) << 24) |
				(uint32_t(c.b * 255) << 16) |
				(uint32_t(c.g * 255) << 8)  |
				(uint32_t(c.r * 255) << 0);

			// 텍스처
			spine::TextureRegion* texRegion = mesh->getRegion();
			if(!texRegion) continue;

			auto* atlasRegion = reinterpret_cast<spine::AtlasRegion*>(texRegion);
			auto* texSRV = reinterpret_cast<ID3D11ShaderResourceView*>(atlasRegion->page->texture);
			if(!texSRV) continue;

			// 위치 변환 및 복사
			{
				size_t bufSize = mesh->getWorldVerticesLength();
				D3D11_MAPPED_SUBRESOURCE mapped ={};
				if(SUCCEEDED(d3dContext->Map(curSqc->bufPos,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))) {
					float* ptr = (float*)mapped.pData;
					mesh->computeWorldVertices(*slot,0, bufSize, ptr, 0, 2);
					d3dContext->Unmap(curSqc->bufPos,0);
				}
			}
			// color
			{
				size_t vtxCount = mesh->getWorldVerticesLength()/2;
				D3D11_MAPPED_SUBRESOURCE mapped ={};
				if(SUCCEEDED(d3dContext->Map(curSqc->bufDif,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))) {
					G2::avx2_memset32((uint32_t*)mapped.pData, rgba, vtxCount);
					d3dContext->Unmap(curSqc->bufDif,0);
				}
			}
			// 텍스처 좌표 복사
			{
				const float* uvs = mesh->getUVs().buffer();
				auto bufSize = mesh->getUVs().size();
				D3D11_MAPPED_SUBRESOURCE mapped ={};
				if(SUCCEEDED(d3dContext->Map(curSqc->bufTex,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))) {
					G2::avx2_memcpy(mapped.pData,uvs,sizeof(float) * bufSize);
					d3dContext->Unmap(curSqc->bufTex,0);
				}
			}
			// index buffer
			const uint16_t* indices = mesh->getTriangles().buffer();
			UINT indexCount = mesh->getTriangles().size();
			{
				D3D11_MAPPED_SUBRESOURCE mapped ={};
				if(SUCCEEDED(d3dContext->Map(curSqc->bufIdx,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))) {
					G2::avx2_memcpy(mapped.pData,indices,sizeof(uint16_t) * indexCount);
					d3dContext->Unmap(curSqc->bufIdx,0);
				}
			}
		}
		else if(curSqc->meshType == VtxSequenceSpine::ESPINE_ATTACHMENT_TYPE::ESPINE_MESH_REGION)
		{
			auto* region = static_cast<spine::RegionAttachment*>(attachment);

			// RegionAttachment → TextureRegion
			spine::TextureRegion* texRegion = region->getRegion();
			if(!texRegion)
				continue;

			// TextureRegion → AtlasRegion
			auto* atlasRegion = reinterpret_cast<spine::AtlasRegion*>(texRegion);

			// AtlasPage → rendererObject
			auto* page = atlasRegion->page;
			auto* texSRV = reinterpret_cast<ID3D11ShaderResourceView*>(page->texture);
			if(!texSRV)
				continue;

			// 색상 계산
			spine::Color c = m_spineSkeleton->getColor();
			c.a *= slot->getColor().a;
			c.r *= slot->getColor().r;
			c.g *= slot->getColor().g;
			c.b *= slot->getColor().b;

			spine::Color rColor = region->getColor();
			c.a *= rColor.a;
			c.r *= rColor.r;
			c.g *= rColor.g;
			c.b *= rColor.b;

			uint32_t rgba =
				(uint32_t(c.a * 255) << 24) |
				(uint32_t(c.r * 255) << 16) |
				(uint32_t(c.g * 255) << 8)  |
				(uint32_t(c.b * 255) << 0);

			// 8. 렌더링 정점 복사
			// 위치 변환 및 복사
			{
				size_t vtxCount = 8;
				D3D11_MAPPED_SUBRESOURCE mapped ={};
				if(SUCCEEDED(d3dContext->Map(curSqc->bufPos,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))) {
					float* ptr = (float*)mapped.pData;
					region->computeWorldVertices(*slot,ptr,0,2);
					d3dContext->Unmap(curSqc->bufPos,0);
				}
			}
			// 텍스처 좌표 복사
			{
				D3D11_MAPPED_SUBRESOURCE mapped ={};
				const float* uvs = region->getUVs().buffer();
				auto uvSize = region->getUVs().size();
				if(SUCCEEDED(d3dContext->Map(curSqc->bufTex,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))) {
					float* ptr = (float*)mapped.pData;
					G2::avx2_memcpy(mapped.pData,uvs,sizeof(float) * uvSize);
					d3dContext->Unmap(curSqc->bufTex,0);
				}
			}
			// color
			{
				size_t vtxCount = 4;
				D3D11_MAPPED_SUBRESOURCE mapped ={};
				if(SUCCEEDED(d3dContext->Map(curSqc->bufDif,0,D3D11_MAP_WRITE_DISCARD,0,&mapped))) {
					uint32_t* ptr = (uint32_t*)mapped.pData;
					G2::avx2_memset32(ptr,rgba,vtxCount);
					d3dContext->Unmap(curSqc->bufDif,0);
				}
			}
		}
	}
}

void SceneSpine::SetupSpineSequence(int order, void* attachment, VtxSequenceSpine::ESPINE_ATTACHMENT_TYPE attachmentType)
{
	size_t vertexCount = 4;
	UINT   indexCount  = 0;
	VtxSequenceSpine* sequenceCur{};
	auto itr = m_spineSequence.find(order);
	if(itr != m_spineSequence.end())
	{
		sequenceCur = itr->second;
	}

	// 메시 타입 체크.
	if(VtxSequenceSpine::ESPINE_ATTACHMENT_TYPE::ESPINE_MESH_ATTACH == attachmentType)
	{
		auto* mesh = static_cast<spine::MeshAttachment*>(attachment);
		vertexCount = mesh->getWorldVerticesLength()/2;
		indexCount = mesh->getTriangles().size();
	}
	else if(VtxSequenceSpine::ESPINE_ATTACHMENT_TYPE::ESPINE_MESH_REGION == attachmentType)
	{
		vertexCount = 4;
		indexCount  = 0;
	}
	else
	{
		// 존재하는데 어느 타입도 아니라면 삭제
		if(sequenceCur)
		{
			delete sequenceCur;
		}
		m_spineSequence.erase(itr);
	}

	// 시퀀스 생성
	if(!sequenceCur)
	{
		sequenceCur = new VtxSequenceSpine;
		m_spineSequence.emplace(order,sequenceCur);
	}
	else if(   sequenceCur->meshType == attachmentType				// 같은 메시 타입
			&& sequenceCur->countVtx == vertexCount					// 같은 버텍스 크기
			&& sequenceCur->countIdx == indexCount					// 같은 인덱스 크기
	)
	{
		// 동일 정보라 할 일이 없음.
		return;
	}
	else
	{
		// 삭제만
		delete sequenceCur;
		sequenceCur = new VtxSequenceSpine;
		m_spineSequence[order] = sequenceCur;
	}
	// 새로운 버텍스 인덱스 생성
	sequenceCur->resourceBinding(order, attachment, attachmentType, vertexCount, indexCount);
}

void SceneSpine::load(spine::AtlasPage& page,const spine::String& path) {
	auto fileName = path.buffer();
	HRESULT hr = S_OK;
	auto d3dDevice  = std::any_cast<ID3D11Device*>(IG2GraphicsD3D::getInstance()->GetDevice());
	auto d3dContext = std::any_cast<ID3D11DeviceContext*>(IG2GraphicsD3D::getInstance()->GetContext());
	ID3D11Resource*				textureRsc{};
	ID3D11ShaderResourceView*	textureView{};
	auto wstr_file = G2::ansiToWstr(fileName);
	hr  = DirectX::CreateWICTextureFromFile(d3dDevice, d3dContext, wstr_file.c_str(), &textureRsc, &textureView);
	if(SUCCEEDED(hr))
	{
		m_spineTexture = textureView;
		page.texture = m_spineTexture;
	}
}

void SceneSpine::unload(void* texture) {
	texture = nullptr;
}

