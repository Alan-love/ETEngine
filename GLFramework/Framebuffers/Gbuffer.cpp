#include "stdafx.hpp"
#include "Gbuffer.hpp"

#include "../Graphics/ShaderData.hpp"

Gbuffer::Gbuffer(bool demo):
	FrameBuffer(demo?
		"Resources/Shaders/bufferDisplayShader.glsl":
		"Resources/Shaders/deferredUberShader.glsl", 
		GL_FLOAT, 4)
{
}
Gbuffer::~Gbuffer()
{
}

void Gbuffer::AccessShaderAttributes()
{
	glUniform1i(glGetUniformLocation(m_pShader->GetProgram(), "texPosition"), 0);
	glUniform1i(glGetUniformLocation(m_pShader->GetProgram(), "texNormal"), 1);
	glUniform1i(glGetUniformLocation(m_pShader->GetProgram(), "texDiffuse"), 2);
	glUniform1i(glGetUniformLocation(m_pShader->GetProgram(), "texSpecular"), 3);

	m_uLightDir = glGetUniformLocation(m_pShader->GetProgram(), "lightDir");
	m_uCamPos = glGetUniformLocation(m_pShader->GetProgram(), "camPos");

	m_uAmbCol = glGetUniformLocation(m_pShader->GetProgram(), "ambientColor");
}
void Gbuffer::UploadDerivedVariables()
{
	glUniform3f(m_uLightDir, m_LightDir.x, m_LightDir.y, m_LightDir.z);
	glm::vec3 cPos = CAMERA->GetTransform()->GetPosition();
	glUniform3f(m_uCamPos, cPos.x, cPos.y, cPos.z);

	glUniform3f(m_uAmbCol, m_AmbientColor.x, m_AmbientColor.y, m_AmbientColor.z);
}