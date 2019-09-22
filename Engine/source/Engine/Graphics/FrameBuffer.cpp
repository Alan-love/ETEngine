#include "stdafx.h"
#include "FrameBuffer.h"

#include <glad/glad.h>

#include "Shader.h"
#include "TextureData.h"

#include <EtCore/FileSystem/FileUtil.h>
#include <EtCore/Content/ResourceManager.h>

#include <Engine/GraphicsHelper/PrimitiveRenderer.h>


FrameBuffer::FrameBuffer(std::string shaderFile, GLenum format, uint32 numTargets)
	:m_ShaderFile(shaderFile),
	m_Format(format),
	m_NumTargets(numTargets)
{
}
FrameBuffer::~FrameBuffer()
{
	GraphicsApiContext* const api = Viewport::GetCurrentApiContext();

	api->DeleteRenderBuffers(1, &m_RboDepthStencil);
	for (size_t i = 0; i < m_pTextureVec.size(); i++)
	{
		SafeDelete(m_pTextureVec[i]);
	}
	api->DeleteFramebuffers(1, &m_GlFrameBuffer);
}

void FrameBuffer::Initialize()
{
	GraphicsApiContext* const api = Viewport::GetCurrentApiContext();

	//Load and compile Shaders
	m_pShader = ResourceManager::Instance()->GetAssetData<ShaderData>(GetHash(FileUtil::ExtractName(m_ShaderFile)));

	//GetAccessTo shader attributes
	api->SetShader(m_pShader.get());
	AccessShaderAttributes();

	//FrameBuffer
	api->GenFramebuffers(1, &m_GlFrameBuffer);

	GenerateFramebufferTextures();

	ET_ASSERT(api->IsFramebufferComplete(), "Creating framebuffer failed!");

	Config::GetInstance()->GetWindow().WindowResizeEvent.AddListener(std::bind( &FrameBuffer::ResizeFramebufferTextures, this));
}

void FrameBuffer::AccessShaderAttributes()
{
	m_pShader->Upload("texFramebuffer"_hash, 0);
}

void FrameBuffer::Enable(bool active)
{
	Viewport::GetCurrentApiContext()->BindFramebuffer(active ? m_GlFrameBuffer : 0);
}

void FrameBuffer::Draw()
{
	GraphicsApiContext* const api = Viewport::GetCurrentApiContext();

	api->SetDepthEnabled(false);
	api->SetShader(m_pShader.get());

	for (uint32 i = 0; i < (uint32)m_pTextureVec.size(); i++)
	{
		api->LazyBindTexture(i, E_TextureType::Texture2D, m_pTextureVec[i]->GetHandle());
	}

	UploadDerivedVariables();

	PrimitiveRenderer::GetInstance()->Draw<primitives::Quad>();
}


void FrameBuffer::GenerateFramebufferTextures()
{
	GraphicsApiContext* const api = Viewport::GetCurrentApiContext();

	Config::Settings::Window const& windowSettings = Config::GetInstance()->GetWindow();

	api->BindFramebuffer(m_GlFrameBuffer);

	//Textures
	m_pTextureVec.reserve(m_NumTargets);

	TextureParameters params(false);
	params.wrapS = E_TextureWrapMode::ClampToEdge;
	params.wrapT = E_TextureWrapMode::ClampToEdge;
	//Depth buffer
	if (m_CaptureDepth)
	{
		TextureData* depthMap = new TextureData(windowSettings.Width, windowSettings.Height, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT );
		depthMap->Build();
		api->LinkTextureToFboDepth(depthMap->GetHandle());
		depthMap->SetParameters(params);
		m_pTextureVec.emplace_back(depthMap);
	}

	//Color buffers
	for (uint32 i = 0; i < m_NumTargets; i++)
	{
		TextureData* colorBuffer = new TextureData(windowSettings.Width, windowSettings.Height, GL_RGBA16F, GL_RGBA, m_Format );
		colorBuffer->Build();
		api->LinkTextureToFbo2D(i, colorBuffer->GetHandle(), 0);
		colorBuffer->SetParameters(params, true);
		m_pTextureVec.emplace_back(colorBuffer);
	}

	//Render Buffer for depth and stencil
	if (!m_CaptureDepth)
	{
		api->GenRenderBuffers(1, &m_RboDepthStencil);
		api->BindRenderbuffer(m_RboDepthStencil);
		api->SetRenderbufferStorage(GL_DEPTH24_STENCIL8, windowSettings.Dimensions);
		api->LinkRenderbufferToFbo(GL_DEPTH_STENCIL_ATTACHMENT, m_RboDepthStencil);
	}

	api->SetDrawBufferCount(static_cast<size_t>(m_NumTargets));
}

void FrameBuffer::ResizeFramebufferTextures()
{
	GraphicsApiContext* const api = Viewport::GetCurrentApiContext();

	Config::Settings::Window const& windowSettings = Config::GetInstance()->GetWindow();

	ET_ASSERT(m_pTextureVec.size() > 0);

	if(windowSettings.Width > m_pTextureVec[0]->GetResolution().x || windowSettings.Height > m_pTextureVec[0]->GetResolution().y)
	{
		api->DeleteRenderBuffers( 1, &m_RboDepthStencil );
		for(uint32 i = 0; i < m_pTextureVec.size(); i++)
		{
			SafeDelete( m_pTextureVec[i] );
		}
		m_pTextureVec.clear();
		api->DeleteFramebuffers( 1, &m_GlFrameBuffer );
		api->GenFramebuffers( 1, &m_GlFrameBuffer );
		GenerateFramebufferTextures();
		return;
	}

	uint32 offset = 0;
	if(m_CaptureDepth)
	{
		m_pTextureVec[0]->Resize(windowSettings.Dimensions);
		++offset;
	}
	else 
	{
		//completely regenerate the renderbuffer object
		api->BindFramebuffer(m_GlFrameBuffer);
		api->DeleteRenderBuffers( 1, &m_RboDepthStencil );
		api->GenRenderBuffers(1, &m_RboDepthStencil);
		api->BindRenderbuffer(m_RboDepthStencil);
		api->SetRenderbufferStorage(GL_DEPTH24_STENCIL8, windowSettings.Dimensions);
		api->LinkRenderbufferToFbo(GL_DEPTH_STENCIL_ATTACHMENT, m_RboDepthStencil);
	}
	assert( m_pTextureVec.size() >= offset + m_NumTargets );
	for(uint32 i = offset; i < offset + m_NumTargets; ++i)
	{
		m_pTextureVec[i]->Resize(windowSettings.Dimensions);
	}
}
