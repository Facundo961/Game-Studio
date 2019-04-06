#include "GBufferRenderPass.h"

#include <GLAD/glad.h>
#include "GL.h"

#include "Renderer.h"
#include "RenderProxy.h"
#include "WorldObject.h"

#include "GSM.hpp"

GBufferRenderPass::GBufferRenderPass(Renderer * RendererOwner) : RenderPass(RendererOwner), Position(ImageSize(1280, 720), GL_RGB16F, GL_RGB, GL_FLOAT), Normal(ImageSize(1280, 720), GL_RGB16F, GL_RGB, GL_FLOAT), Albedo(ImageSize(1280, 720), GL_RGBA, GL_RGB, GL_UNSIGNED_BYTE), Depth(ImageSize(1280, 720), GL_DEPTH_STENCIL, GL_DEPTH_COMPONENT, GL_FLOAT), GBuffer(3), GBufferPassProgram("W:/Game Studio/GS_Engine/src/Game Studio/GBufferVS.vshader", "W:/Game Studio/GS_Engine/src/Game Studio/GBufferFS.fshader"), ViewMatrix(GBufferPassProgram, "uView"), ProjMatrix(GBufferPassProgram, "uProjection"), ModelMatrix(GBufferPassProgram, "uModel")
{
	//Bind the GBuffer frame buffer so all subsequent texture attachment calls are done on this frame buffer.
	GBuffer.Bind();

	//Attach textures to the bound frame buffer.
	GBuffer.AttachTexture(Position);		//Position Texture.
	GBuffer.AttachTexture(Normal);			//Normal Texture.
	GBuffer.AttachTexture(Albedo);			//Albedo Texture.
	GBuffer.AttachDepthTexture(Depth);		//Depth Texture.
}

GBufferRenderPass::~GBufferRenderPass()
{
}

void GBufferRenderPass::Render()
{
	GBufferPassProgram.Bind();

	GBuffer.BindForWrite();
	GBuffer.SetAsDrawBuffer();
	FBO::Clear();

	ViewMatrix.Set(RendererOwner->GetScene()->GetViewMatrix());
	ProjMatrix.Set(RendererOwner->GetScene()->GetProjectionMatrix());


	DrawCalls = RendererOwner->GetScene()->RenderProxyList.length();

	for (size_t i = 0; i < DrawCalls; i++)
	{
		ModelMatrix.Set(GSM::Translation(RendererOwner->GetScene()->RenderProxyList[i]->GetOwner()->GetPosition()));

		RendererOwner->GetScene()->RenderProxyList[i]->Draw();
	}

	return;
}