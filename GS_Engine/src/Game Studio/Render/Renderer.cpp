#include "Renderer.h"
#include "RAPI/RenderDevice.h"

#include "Application/Application.h"
#include "Math/GSM.hpp"

#include "Resources/StaticMeshResource.h"
#include "Resources/MaterialResource.h"

#include "Material.h"
#include "Game/StaticMesh.h"
#include "MeshRenderResource.h"
#include "MaterialRenderResource.h"
#include "Resources/TextureResource.h"


Renderer::Renderer() : Framebuffers(3), ViewMatrix(1), ViewProjectionMatrix(1)
{
	Win = GS::Application::Get()->GetActiveWindow();

	RenderContextCreateInfo RCCI;
	RCCI.Window = Win;
	RC = RenderDevice::Get()->CreateRenderContext(RCCI);

	auto SCImages = RC->GetSwapchainImages();

	//ImageCreateInfo CACI;
	//CACI.Extent = GetWindow()->GetWindowExtent();
	//CACI.LoadOperation = LoadOperations::CLEAR;
	//CACI.StoreOperation = StoreOperations::UNDEFINED;
	//CACI.Dimensions = ImageDimensions::IMAGE_2D;
	//CACI.InitialLayout = ImageLayout::COLOR_ATTACHMENT;
	//CACI.FinalLayout = ImageLayout::COLOR_ATTACHMENT;
	//CACI.Use = ImageUse::COLOR_ATTACHMENT;
	//CACI.Type = ImageType::COLOR;
	//CACI.ImageFormat = Format::RGB_I8;
	//auto CA = RAPI::GetRAPI()->CreateImage(CACI);

	RenderPassCreateInfo RPCI;
	RenderPassDescriptor RPD;
	AttachmentDescriptor SIAD;
	SubPassDescriptor SPD;
	AttachmentReference SubPassWriteAttachmentReference;
	AttachmentReference SubPassReadAttachmentReference;

	SIAD.AttachmentImage = SCImages[0]; //Only first because it gets only properties, doesn't access actual data.
	SIAD.InitialLayout = ImageLayout::UNDEFINED;
	SIAD.FinalLayout = ImageLayout::PRESENTATION;
	SIAD.StoreOperation = StoreOperations::STORE;
	SIAD.LoadOperation = LoadOperations::CLEAR;

	SubPassWriteAttachmentReference.Layout = ImageLayout::COLOR_ATTACHMENT;
	SubPassWriteAttachmentReference.Index = 0;

	SubPassReadAttachmentReference.Layout = ImageLayout::GENERAL;
	SubPassReadAttachmentReference.Index = ATTACHMENT_UNUSED;

	SPD.WriteColorAttachments.push_back(&SubPassWriteAttachmentReference);
	SPD.ReadColorAttachments.push_back(&SubPassReadAttachmentReference);

	RPD.RenderPassColorAttachments.push_back(&SIAD);
	RPD.SubPasses.push_back(&SPD);

	RPCI.Descriptor = RPD;
	RP = RenderDevice::Get()->CreateRenderPass(RPCI);

	UniformLayoutCreateInfo ULCI;
	ULCI.RenderContext = RC;
	ULCI.PipelineUniformSets[0].UniformSetType = UniformType::UNIFORM_BUFFER;
	ULCI.PipelineUniformSets[0].ShaderStage = ShaderType::VERTEX_SHADER;
	ULCI.PipelineUniformSets[0].UniformSetUniformsCount = 1;
	ULCI.PipelineUniformSets[1].UniformSetType = UniformType::SAMPLER;
	ULCI.PipelineUniformSets[1].ShaderStage = ShaderType::FRAGMENT_SHADER;
	ULCI.PipelineUniformSets[1].UniformSetUniformsCount = 1;
	ULCI.PipelineUniformSets.setLength(2);
	
	PushConstant MyPushConstant;
	MyPushConstant.Size = sizeof(Matrix4);
	ULCI.PushConstant = &MyPushConstant;
	
	UL = RenderDevice::Get()->CreateUniformLayout(ULCI);

	UniformBufferCreateInfo UBCI;
	UBCI.Size = sizeof(Matrix4);
	UB = RenderDevice::Get()->CreateUniformBuffer(UBCI);

	UniformLayoutUpdateInfo ULUI;
	ULUI.PipelineUniformSets[0].UniformSetType = UniformType::UNIFORM_BUFFER;
	ULUI.PipelineUniformSets[0].ShaderStage = ShaderType::VERTEX_SHADER;
	ULUI.PipelineUniformSets[0].UniformSetUniformsCount = 1;
	ULUI.PipelineUniformSets[0].UniformData = UB;
	ULUI.PipelineUniformSets.setLength(1);
	UL->UpdateUniformSet(ULUI);

	Framebuffers.resize(SCImages.getLength());
	for (uint8 i = 0; i < SCImages.getLength(); ++i)
	{
		FramebufferCreateInfo FBCI;
		FBCI.RenderPass = RP;
		FBCI.Extent = Win->GetWindowExtent();
		FBCI.Images = DArray<Image*>(&SCImages[i], 1);
		Framebuffers[i] = RenderDevice::Get()->CreateFramebuffer(FBCI);
	}

	MeshCreateInfo MCI;
	MCI.IndexCount = ScreenQuad::IndexCount;
	MCI.VertexCount = ScreenQuad::VertexCount;
	MCI.VertexData = ScreenQuad::Vertices;
	MCI.IndexData = ScreenQuad::Indices;
	MCI.VertexLayout = &ScreenQuad::VD;
	FullScreenQuad = RenderDevice::Get()->CreateMesh(MCI);

	GraphicsPipelineCreateInfo gpci;
	gpci.RenderPass = RP;
	gpci.UniformLayout = UL;
	gpci.VDescriptor = &ScreenQuad::VD;
	gpci.PipelineDescriptor.BlendEnable = false;
	gpci.ActiveWindow = Win;
	
	FString VS("#version 450\nlayout(push_constant) uniform Push {\nmat4 Mat;\n} inPush;\nlayout(binding = 0, row_major) uniform Data {\nmat4 Pos;\n} inData;\nlayout(location = 0)in vec3 inPos;\nlayout(location = 1)in vec3 inTexCoords;\nlayout(location = 0)out vec4 tPos;\nvoid main()\n{\ngl_Position = inData.Pos * vec4(inPos, 1.0);\n}");
	gpci.PipelineDescriptor.Stages.push_back(ShaderInfo{ ShaderType::VERTEX_SHADER, &VS });
	
	FString FS("#version 450\nlayout(location = 0)in vec4 tPos;\nlayout(binding = 1) uniform sampler2D texSampler;\nlayout(location = 0) out vec4 outColor;\nvoid main()\n{\noutColor = vec4(1, 1, 1, 1);\n}");
	gpci.PipelineDescriptor.Stages.push_back(ShaderInfo{ ShaderType::FRAGMENT_SHADER, &FS });

	FullScreenRenderingPipeline = RenderDevice::Get()->CreateGraphicsPipeline(gpci);
}

Renderer::~Renderer()
{
	for (auto& Element : ComponentToInstructionsMap)
	{
		delete Element.second;
	}

	for (auto const& x : Pipelines)
	{
		delete x.second;
	}

	delete RP;
	delete RC;
}

void Renderer::OnUpdate()
{
	/*Update debug vars*/
	GS_DEBUG_ONLY(DrawCalls = 0)
	GS_DEBUG_ONLY(InstanceDraws = 0)
	GS_DEBUG_ONLY(PipelineSwitches = 0)
	GS_DEBUG_ONLY(DrawnComponents = 0)
	/*Update debug vars*/
	
	UpdateMatrices();
	
	UniformBufferUpdateInfo uniform_buffer_update_info;
	uniform_buffer_update_info.Data = &ViewProjectionMatrix;
	uniform_buffer_update_info.Size = sizeof(Matrix4);
	UB->UpdateBuffer(uniform_buffer_update_info);

	RC->BeginRecording();	
	
	RenderPassBeginInfo RPBI;
	RPBI.RenderPass = RP;
	RPBI.Framebuffers = Framebuffers.getData();

	RC->BeginRenderPass(RPBI);

	RC->BindUniformLayout(UL);

	//BindPipeline(FullScreenRenderingPipeline);
	//DrawMesh(DrawInfo{ ScreenQuad::IndexCount, 1 }, FullScreenQuad);

	
	UpdateRenderables();
	
	RenderRenderables();

	RC->EndRenderPass(RP);

	RC->EndRecording();

	RC->AcquireNextImage();
	RC->Flush();
	RC->Present();
}

void Renderer::DrawMesh(const DrawInfo& _DrawInfo, MeshRenderResource* Mesh_)
{
	RC->BindMesh(Mesh_->mesh);
	RC->DrawIndexed(_DrawInfo);
	GS_DEBUG_ONLY(++DrawCalls)
	GS_DEBUG_ONLY(InstanceDraws += _DrawInfo.InstanceCount)
}

void Renderer::BindPipeline(GraphicsPipeline* _Pipeline)
{
	RC->BindGraphicsPipeline(_Pipeline);
	GS_DEBUG_ONLY(++PipelineSwitches);
}

MeshRenderResource* Renderer::CreateMesh(StaticMesh* _SM)
{
	MeshRenderResource* NewMesh = nullptr;

	if (Meshes.find(_SM) == Meshes.end())
	{
		Model m = _SM->GetModel();

		MeshCreateInfo MCI;
		MCI.IndexCount = m.IndexCount;
		MCI.VertexCount = m.VertexCount;
		MCI.VertexData = m.VertexArray;
		MCI.IndexData = m.IndexArray;
		MCI.VertexLayout = StaticMeshResource::GetVertexDescriptor();
		MeshRenderResourceCreateInfo mesh_render_resource_create_info;
		mesh_render_resource_create_info.Mesh = RenderDevice::Get()->CreateMesh(MCI);
		NewMesh = new MeshRenderResource(mesh_render_resource_create_info);
	}
	else
	{
		NewMesh = Meshes[_SM];
	}


	return NewMesh;
}

MaterialRenderResource* Renderer::CreateMaterial(Material* Material_)
{
	auto Res = Pipelines.find(Id(Material_->GetMaterialName()));
	
	if (Res == Pipelines.end())
	{
		auto NP = CreatePipelineFromMaterial(Material_);
		Pipelines.insert({ Id(Material_->GetMaterialName()).GetID(), NP });
	}

	MaterialRenderResourceCreateInfo material_render_resource_create_info;
	material_render_resource_create_info.ParentMaterial = Material_;

	for (uint8 i = 0; i < Material_->GetMaterialResource()->GetMaterialData().TextureNames.getLength(); ++i)
	{
		auto texture = GS::Application::Get()->GetResourceManager()->GetResource<TextureResource>(Material_->GetMaterialResource()->GetMaterialData().TextureNames[i]);
		
		TextureCreateInfo texture_create_info;
		texture_create_info.ImageData = texture->GetTextureData().ImageData;
		texture_create_info.ImageDataSize = texture->GetTextureData().imageDataSize;
		texture_create_info.Extent = texture->GetTextureData().TextureDimensions;
		texture_create_info.ImageFormat = texture->GetTextureData().TextureFormat;
		texture_create_info.Layout = ImageLayout::SHADER_READ;
		
		material_render_resource_create_info.textures.push_back(RenderDevice::Get()->CreateTexture(texture_create_info));
	}
	
	return new MaterialRenderResource(material_render_resource_create_info);
}

GraphicsPipeline* Renderer::CreatePipelineFromMaterial(Material* _Mat) const
{
	GraphicsPipelineCreateInfo GPCI;

	GPCI.VDescriptor = StaticMeshResource::GetVertexDescriptor();

	FVector<ShaderInfo> si;
	_Mat->GetRenderingCode(si);

	for (auto& e : si)
	{
		GPCI.PipelineDescriptor.Stages.push_back(e);
	}
	
	GPCI.PipelineDescriptor.BlendEnable = _Mat->GetHasTransparency();
	GPCI.PipelineDescriptor.ColorBlendOperation = BlendOperation::ADD;
	GPCI.PipelineDescriptor.CullMode = _Mat->GetIsTwoSided() ? CullMode::CULL_NONE : CullMode::CULL_BACK;
	GPCI.PipelineDescriptor.DepthCompareOperation = CompareOperation::GREATER;

	GPCI.RenderPass = RP;
	GPCI.UniformLayout = UL;
	GPCI.ActiveWindow = Win;

	return RenderDevice::Get()->CreateGraphicsPipeline(GPCI);
}

void Renderer::UpdateMatrices()
{
	//We get and store the camera's position so as to not access it several times.
	const Vector3 CamPos = GetActiveCamera()->GetPosition();
	
	//We set the view matrix's corresponding component to the inverse of the camera's position to make the matrix a translation matrix in the opposite direction of the camera.
	ViewMatrix(0, 3) = -CamPos.X;
	ViewMatrix(1, 3) = -CamPos.Y;
	ViewMatrix(2, 3) = CamPos.Z;
	

	auto& nfp = GetActiveCamera()->GetNearFarPair();

	//GSM::Scale(ProjectionMatrix, Vector3(0.1, 0.8, 1));
	//GSM::Translate(ProjectionMatrix, Vector3(1, -1, 0));
	
	BuildPerspectiveMatrix(ProjectionMatrix, 45/*GetActiveCamera()->GetFOV()*/, Win->GetAspectRatio(), 1, 1000);
	
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
}

void Renderer::RegisterRenderComponent(RenderComponent* _RC, RenderComponentCreateInfo* _RCCI)
{
	auto ri = _RC->GetRenderableInstructions();
	
	CreateInstanceResourcesInfo CIRI{ _RC, this };
	CIRI.RenderComponentCreateInfo = _RCCI;
	ri->CreateInstanceResources(CIRI);
	
	ComponentToInstructionsMap.insert(std::pair<GS_HASH_TYPE, RenderComponent*>(Id::HashString(_RC->GetRenderableTypeName()), _RC));
}

void Renderer::UpdateRenderables()
{
	for (auto& e : ComponentToInstructionsMap)
	{
		auto ri = e.second->GetRenderableInstructions();

		BindTypeResourcesInfo btpi{ this };
		ri->BindTypeResources(btpi);
	}
}

void Renderer::RenderRenderables()
{
	BindPipeline(Pipelines.begin()->second);

	for (auto& e : ComponentToInstructionsMap)
	{
		auto ri = e.second->GetRenderableInstructions();
		
		DrawInstanceInfo dii{ this, e.second };
		ri->DrawInstance(dii);
	}
}

void Renderer::BuildPerspectiveMatrix(Matrix4& _Matrix, const float _FOV, const float _AspectRatio, const float _Near, const float _Far)
{
	const auto tan_half_fov = GSM::Tangent(GSM::Clamp(_FOV * 0.5f, 0.0f, 90.0f)); //Tangent of half the vertical view angle.
	const auto f = 1 / tan_half_fov;

	//Zero to one
	//Left handed

	_Matrix(0, 0) = f / _AspectRatio;
	_Matrix(0, 1) = 0.f;
	_Matrix(0, 2) = 0.f;
	_Matrix(0, 3) = 0.f;
	
	_Matrix(1, 0) = 0.f;
	_Matrix(1, 1) = -f;
	_Matrix(1, 2) = 0.f;
	_Matrix(1, 3) = 0.f;
	
	_Matrix(2, 0) = 0.f;
	_Matrix(2, 1) = 0.f;
	_Matrix(2, 2) = -((_Far + _Near) / (_Far - _Near));
	_Matrix(2, 3) = -((2.f * _Far * _Near) / (_Far - _Near));
	
	_Matrix(3, 0) = 0.f;
	_Matrix(3, 1) = 0.f;
	_Matrix(3, 2) = -1.f;
	_Matrix(3, 3) = 0.f;
}

void Renderer::MakeOrthoMatrix(Matrix4& _Matrix, const float _Right, const float _Left, const float _Top,
	const float _Bottom, const float _Near, const float _Far)
{
	//Zero to one
	//Left handed

	_Matrix(0, 0) = static_cast<float>(2) / (_Right - _Left);
	_Matrix(1, 1) = static_cast<float>(2) / (_Top - _Bottom);
	_Matrix(2, 2) = static_cast<float>(1) / (_Far - _Near);
	_Matrix(3, 0) = -(_Right + _Left) / (_Right - _Left);
	_Matrix(3, 1) = -(_Top + _Bottom) / (_Top - _Bottom);
	_Matrix(3, 2) = -_Near / (_Far - _Near);
}
