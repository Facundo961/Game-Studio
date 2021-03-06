#include "RenderOrchestrator.h"

#undef MemoryBarrier

#include <GTSL/Math/Math.hpp>
#include <GTSL/Math/Matrix4.h>


#include "LightsRenderGroup.h"
#include "RenderGroup.h"
#include "ByteEngine/Game/GameInstance.h"
#include "ByteEngine/Game/Tasks.h"

#include "MaterialSystem.h"
#include "RenderState.h"
#include "StaticMeshRenderGroup.h"
#include "UIManager.h"
#include "ByteEngine/Application/Application.h"
#include "ByteEngine/Game/CameraSystem.h"

static constexpr GTSL::Vector2 SQUARE_VERTICES[] = { { -0.5f, 0.5f }, { 0.5f, 0.5f }, { 0.5f, -0.5f }, { -0.5f, -0.5f } };
//static constexpr GTSL::Vector2 SQUARE_VERTICES[] = { { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, -1.0f } };
static constexpr uint16 SQUARE_INDICES[] = { 0, 1, 3, 1, 2, 3 };

void StaticMeshRenderManager::Initialize(const InitializeInfo& initializeInfo)
{
	auto* renderSystem = initializeInfo.GameInstance->GetSystem<RenderSystem>("RenderSystem");
	auto* materialSystem = initializeInfo.GameInstance->GetSystem<MaterialSystem>("MaterialSystem");
	auto* renderOrchestrator = initializeInfo.GameInstance->GetSystem<RenderOrchestrator>("RenderOrchestrator");

	//MaterialSystem::SetInfo setInfo;
	//
	GTSL::Array<MaterialSystem::MemberInfo, 8> members(1);
	members[0].Type = MaterialSystem::Member::DataType::MATRIX4;
	members[0].Handle = &matrixUniformBufferMemberHandle;
	members[0].Count = 16;
	//
	//setInfo.Structs = structs;
	//
	//dataSet = materialSystem->AddSet(renderSystem, "StaticMeshRenderGroup", "SceneRenderPass", setInfo);
	//

	//TODO: ADD DATA
	
	//TODO: MAKE A CORRECT PATH FOR DECLARING RENDER PASSES

	auto bufferHandle = materialSystem->CreateBuffer(renderSystem, members);
	materialSystem->BindBufferToName(bufferHandle, "StaticMeshRenderGroup");
	renderOrchestrator->AddToRenderPass("SceneRenderPass", "StaticMeshRenderGroup");
	//renderOrchestrator->AddIndexStreamToRenderGroup("StaticMeshRenderGroup");
	//renderOrchestrator->AddSetToRenderGroup("StaticMeshRenderGroup", "StaticMeshRenderGroup");
}

void StaticMeshRenderManager::GetSetupAccesses(GTSL::Array<TaskDependency, 16>& dependencies)
{
	dependencies.EmplaceBack(TaskDependency{ "StaticMeshRenderGroup", AccessTypes::READ });
}

void StaticMeshRenderManager::Setup(const SetupInfo& info)
{
	auto* const renderGroup = info.GameInstance->GetSystem<StaticMeshRenderGroup>("StaticMeshRenderGroup");
	auto positions = renderGroup->GetPositions();
	
	//info.RenderOrchestrator->AddMesh(0, {});
	
	info.MaterialSystem->UpdateObjectCount(info.RenderSystem, matrixUniformBufferMemberHandle, renderGroup->GetStaticMesheCount());

	for (uint32 p = 0; p < renderGroup->GetAddedMeshes().GetPageCount(); ++p)
	{
		for(auto e : renderGroup->GetAddedMeshes().GetPage(p))
		{
			info.RenderOrchestrator->AddMesh(e.First, info.RenderSystem->GetMeshMaterialHandle(e.First()), e.Second);
		}
	}

	renderGroup->ClearAddedMeshes();
	
	MaterialSystem::BufferIterator bufferIterator;
	info.MaterialSystem->UpdateIteratorMember(bufferIterator, matrixUniformBufferMemberHandle);
	
	{
		uint32 index = 0;

		for (auto& e : positions)
		{
			auto pos = GTSL::Matrix4(e);
			pos(2, 3) *= -1.f;
			
			//*info.MaterialSystem->GetMemberPointer<GTSL::Matrix4>(bufferIterator) = info.ProjectionMatrix * info.ViewMatrix * pos;
			*info.MaterialSystem->GetMemberPointer<GTSL::Matrix4>(bufferIterator) = pos;
			info.MaterialSystem->UpdateIteratorMemberIndex(bufferIterator, ++index);
		}
	}

	//if ray tracing
	//info.RenderSystem->SetMeshMatrix();
	//clear updated meshes
}

void UIRenderManager::Initialize(const InitializeInfo& initializeInfo)
{
	auto* renderSystem = initializeInfo.GameInstance->GetSystem<RenderSystem>("RenderSystem");
	auto* materialSystem = initializeInfo.GameInstance->GetSystem<MaterialSystem>("MaterialSystem");
	auto* renderOrchestrator = initializeInfo.GameInstance->GetSystem<RenderOrchestrator>("RenderOrchestrator");
	
	//auto mesh = renderSystem->CreateMesh("BE_UI_SQUARE", 4, 4 * 2, 6, 2, materialSystem->GetMaterialHandle("UIMat"));
	//
	//auto* meshPointer = renderSystem->GetMeshPointer(mesh);
	//GTSL::MemCopy(4 * 2 * 4, SQUARE_VERTICES, meshPointer);
	//meshPointer += 4 * 2 * 4;
	//GTSL::MemCopy(6 * 2, SQUARE_INDICES, meshPointer);
	//
	//square = renderSystem->UpdateMesh(mesh);
	
	//MaterialSystem::CreateMaterialInfo createMaterialInfo;
	//createMaterialInfo.RenderSystem = renderSystem;
	//createMaterialInfo.GameInstance = initializeInfo.GameInstance;
	//createMaterialInfo.MaterialName = "UIMat";
	//createMaterialInfo.MaterialResourceManager = BE::Application::Get()->GetResourceManager<MaterialResourceManager>("MaterialResourceManager");
	//createMaterialInfo.TextureResourceManager = BE::Application::Get()->GetResourceManager<TextureResourceManager>("TextureResourceManager");
	//uiMaterial = materialSystem->CreateRasterMaterial(createMaterialInfo);

	//MaterialSystem::SetInfo setInfo;
	//
	//GTSL::Array<MaterialSystem::MemberInfo, 8> members(2);
	//members[0].Type = MaterialSystem::Member::DataType::MATRIX4;
	//members[0].Handle = &matrixUniformBufferMemberHandle;
	//members[0].Count = 16;
	//
	//members[1].Type = MaterialSystem::Member::DataType::FVEC4;
	//members[1].Handle = &colorHandle;
	//members[1].Count = 16;
	//
	//GTSL::Array<MaterialSystem::StructInfo, 4> structs(1);
	//structs[0].Members = members;
	//
	//setInfo.Structs = structs;
	//
	//dataSet = materialSystem->AddSet(renderSystem, "UIRenderGroup", "UIRenderPass", setInfo);
	//TODO: MAKE A CORRECT PATH FOR DECLARING RENDER PASSES

	renderOrchestrator->AddToRenderPass("UIRenderPass", "UIRenderGroup");
}

void UIRenderManager::GetSetupAccesses(GTSL::Array<TaskDependency, 16>& dependencies)
{
	dependencies.EmplaceBack(TaskDependency{ "UIManager", AccessTypes::READ });
	dependencies.EmplaceBack(TaskDependency{ "CanvasSystem", AccessTypes::READ });
}

void UIRenderManager::Setup(const SetupInfo& info)
{
	auto* uiSystem = info.GameInstance->GetSystem<UIManager>("UIManager");
	auto* canvasSystem = info.GameInstance->GetSystem<CanvasSystem>("CanvasSystem");

	float32 scale = 1.0f;

	auto canvases = uiSystem->GetCanvases();

	info.MaterialSystem->UpdateObjectCount(info.RenderSystem, matrixUniformBufferMemberHandle, comps);
	
	for (auto& ref : canvases)
	{
		auto& canvas = canvasSystem->GetCanvas(ref);
		auto canvasSize = canvas.GetExtent();

		float xyRatio = static_cast<float32>(canvasSize.Width) / static_cast<float32>(canvasSize.Height);
		float yxRatio = static_cast<float32>(canvasSize.Height) / static_cast<float32>(canvasSize.Width);
		
		GTSL::Matrix4 ortho(1.0f);
		GTSL::Math::MakeOrthoMatrix(ortho, 1.0f,
		                            -1.0f,
		                            yxRatio,
		                            -yxRatio, 0, 100);

		//GTSL::Math::MakeOrthoMatrix(ortho, canvasSize.Width, -canvasSize.Width, canvasSize.Height, -canvasSize.Height, 0, 100);
		
		//GTSL::Math::MakeOrthoMatrix(ortho, 0.5f, -0.5f, 0.5f, -0.5f, 1, 100);
		
		auto& organizers = canvas.GetOrganizersTree();

		auto primitives = canvas.GetPrimitives();
		auto squares = canvas.GetSquares();

		const auto* parentOrganizer = organizers[0];

		uint32 sq = 0;
		for(auto& e : squares)
		{
			GTSL::Matrix4 trans(1.0f);

			auto location = primitives.begin()[e.PrimitiveIndex].RelativeLocation;
			auto scale = primitives.begin()[e.PrimitiveIndex].AspectRatio;
			//
			GTSL::Math::Translate(trans, GTSL::Vector3(location.X(), -location.Y(), 0));
			GTSL::Math::Scale(trans, GTSL::Vector3(scale.X(), scale.Y(), 1));
			//GTSL::Math::Scale(trans, GTSL::Vector3(static_cast<float32>(canvasSize.Width), static_cast<float32>(canvasSize.Height), 1));
			//
			
			//*info.MaterialSystem->GetMemberPointer<GTSL::Matrix4>(matrixUniformBufferMemberHandle, sq) = trans * ortho;
			//*reinterpret_cast<GTSL::RGBA*>(info.MaterialSystem->GetMemberPointer<GTSL::Vector4>(colorHandle, sq)) = uiSystem->GetColor(e.GetColor());
			++sq;
		}
		
		//auto processNode = [&](decltype(parentOrganizer) node, uint32 depth, GTSL::Matrix4 parentTransform, auto&& self) -> void
		//{
		//	GTSL::Matrix4 transform;
		//
		//	for (uint32 i = 0; i < node->Nodes.GetLength(); ++i) { self(node->Nodes[i], depth + 1, transform, self); }
		//
		//	const auto aspectRatio = organizersAspectRatio.begin()[parentOrganizer->Data];
		//	GTSL::Matrix4 organizerMatrix = ortho;
		//	GTSL::Math::Scale(organizerMatrix, { aspectRatio.X, aspectRatio.Y, 1.0f });
		//
		//	for (auto square : organizersSquares.begin()[node->Data])
		//	{
		//		primitivesPerOrganizer->begin()[square.PrimitiveIndex].AspectRatio;
		//	}
		//};
		//
		//processNode(parentOrganizer, 0, ortho, processNode);
	}

	//if (textSystem->GetTexts().ElementCount())
	//{
	//	int32 atlasIndex = 0;
	//	
	//	auto& text = textSystem->GetTexts()[0];
	//	auto& imageFont = textSystem->GetFont();
	//
	//	auto x = text.Position.X;
	//	auto y = text.Position.Y;
	//	
	//	byte* data = static_cast<byte*>(info.MaterialSystem->GetRenderGroupDataPointer("TextSystem"));
	//
	//	uint32 offset = 0;
	//	
	//	GTSL::Matrix4 ortho;
	//	auto renderExtent = info.RenderSystem->GetRenderExtent();
	//	GTSL::Math::MakeOrthoMatrix(ortho, static_cast<float32>(renderExtent.Width) * 0.5f, static_cast<float32>(renderExtent.Width) * -0.5f, static_cast<float32>(renderExtent.Height) * 0.5f, static_cast<float32>(renderExtent.Height) * -0.5f, 1, 100);
	//	GTSL::MemCopy(sizeof(ortho), &ortho, data + offset); offset += sizeof(ortho);
	//	GTSL::MemCopy(sizeof(uint32), &atlasIndex, data + offset); offset += sizeof(uint32); offset += sizeof(uint32) * 3;
	//	
	//	for (auto* c = text.String.begin(); c != text.String.end() - 1; c++)
	//	{
	//		auto& ch = imageFont.Characters.at(*c);
	//
	//		float xpos = x + ch.Bearing.X * scale;
	//		float ypos = y - (ch.Size.Height - ch.Bearing.Y) * scale;
	//
	//		float w = ch.Size.Width * scale;
	//		float h = ch.Size.Height * scale;
	//		
	//		// update VBO for each character
	//		float vertices[6][4] = {
	//			{ xpos,     -(ypos + h),   0.0f, 0.0f },
	//			{ xpos,     -(ypos),       0.0f, 1.0f },
	//			{ xpos + w, -(ypos),       1.0f, 1.0f },
	//
	//			{ xpos,     -(ypos + h),   0.0f, 0.0f },
	//			{ xpos + w, -(ypos),       1.0f, 1.0f },
	//			{ xpos + w, -(ypos + h),   1.0f, 0.0f }
	//		};
	//		
	//		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
	//		x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
	//
	//		uint32 val = ch.Position.Width;
	//		GTSL::MemCopy(sizeof(val), &val, data + offset); offset += sizeof(val);
	//		val = ch.Position.Height;
	//		GTSL::MemCopy(sizeof(val), &val, data + offset); offset += sizeof(val);
	//
	//		val = ch.Size.Width;
	//		GTSL::MemCopy(sizeof(val), &val, data + offset); offset += sizeof(val);
	//		val = ch.Size.Height;
	//		GTSL::MemCopy(sizeof(val), &val, data + offset); offset += sizeof(val);
	//		
	//		for (uint32 v = 0; v < 6; ++v)
	//		{
	//			GTSL::MemCopy(sizeof(GTSL::Vector2), &vertices[v][0], data + offset); offset += sizeof(GTSL::Vector2); //vertices
	//			GTSL::MemCopy(sizeof(GTSL::Vector2), &vertices[v][2], data + offset); offset += sizeof(GTSL::Vector2); //uv
	//		}
	//		
	//	}
	//
	//}
	////MaterialSystem::UpdateRenderGroupDataInfo updateInfo;
	////updateInfo.RenderGroup = "TextSystem";
	////updateInfo.Data = GTSL::Range<const byte>(64, static_cast<const byte*>(nullptr));
	////updateInfo.Offset = 0;
	////info.MaterialSystem->UpdateRenderGroupData(updateInfo);
}

void RenderOrchestrator::Initialize(const InitializeInfo& initializeInfo)
{
	systems.Initialize(32, GetPersistentAllocator());
	
	{
		const GTSL::Array<TaskDependency, 4> dependencies{ { CLASS_NAME, AccessTypes::READ_WRITE } };
		initializeInfo.GameInstance->AddTask(SETUP_TASK_NAME, GTSL::Delegate<void(TaskInfo)>::Create<RenderOrchestrator, &RenderOrchestrator::Setup>(this), dependencies, "GameplayEnd", "RenderStart");
		initializeInfo.GameInstance->AddTask(RENDER_TASK_NAME, GTSL::Delegate<void(TaskInfo)>::Create<RenderOrchestrator, &RenderOrchestrator::Render>(this), dependencies, "RenderDo", "RenderFinished");
	}

	renderPassesMap.Initialize(8, GetPersistentAllocator());
	renderManagers.Initialize(16, GetPersistentAllocator());
	setupSystemsAccesses.Initialize(16, GetPersistentAllocator());

	renderPassesFunctions.Emplace(Id("SceneRenderPass"), RenderPassFunctionType::Create<RenderOrchestrator, &RenderOrchestrator::renderScene>());
	renderPassesFunctions.Emplace(Id("UIRenderPass"), RenderPassFunctionType::Create<RenderOrchestrator, &RenderOrchestrator::renderUI>());
	renderPassesFunctions.Emplace(Id("SceneRTRenderPass"), RenderPassFunctionType::Create<RenderOrchestrator, &RenderOrchestrator::renderRays>());

	//auto onMaterialLoadHandle = initializeInfo.GameInstance->StoreDynamicTask("OnMaterialLoad", Task<Id>::Create<RenderOrchestrator, &RenderOrchestrator::onMaterialLoad>(this), GTSL::Array<TaskDependency, 4>{ { "RenderOrchestrator", AccessTypes::READ_WRITE } });
	//initializeInfo.GameInstance->SubscribeToEvent("MaterialSystem", MaterialSystem::GetOnMaterialLoadEventHandle(), onMaterialLoadHandle);
	//
	//auto onMaterialInstanceLoadHandle = initializeInfo.GameInstance->StoreDynamicTask("OnMaterialInstanceLoad", Task<Id, Id>::Create<RenderOrchestrator, &RenderOrchestrator::onMaterialInstanceLoad>(this), GTSL::Array<TaskDependency, 4>{ { "RenderOrchestrator", AccessTypes::READ_WRITE } });
	//initializeInfo.GameInstance->SubscribeToEvent("MaterialSystem", MaterialSystem::GetOnMaterialInstanceLoadEventHandle(), onMaterialInstanceLoadHandle);

	auto* renderSystem = initializeInfo.GameInstance->GetSystem<RenderSystem>("RenderSystem");
	auto* materialSystem = initializeInfo.GameInstance->GetSystem<MaterialSystem>("MaterialSystem");

	// MATERIALS

	materials.Initialize(16, GetPersistentAllocator());
	materialInstances.Initialize(32, GetPersistentAllocator());
	loadedMaterialInstances.Initialize(32, GetPersistentAllocator());
	awaitingMaterialInstances.Initialize(8, GetPersistentAllocator());
	loadedMaterials.Initialize(32, GetPersistentAllocator());
	materialInstancesByName.Initialize(32, GetPersistentAllocator());
	
	texturesRefTable.Initialize(32, GetPersistentAllocator());
	pendingMaterialsPerTexture.Initialize(8, GetPersistentAllocator());
	latestLoadedTextures.Initialize(8, GetPersistentAllocator());
	 
	// MATERIALS
	
	{
		const auto taskDependencies = GTSL::Array<TaskDependency, 4>{ { "RenderSystem", AccessTypes::READ_WRITE }, { "RenderOrchestrator", AccessTypes::READ_WRITE } };
		onTextureInfoLoadHandle = initializeInfo.GameInstance->StoreDynamicTask("onTextureInfoLoad", Task<TextureResourceManager*, TextureResourceManager::TextureInfo, TextureLoadInfo>::Create<RenderOrchestrator, &RenderOrchestrator::onTextureInfoLoad>(this), taskDependencies);
	}

	{

		const auto taskDependencies = GTSL::Array<TaskDependency, 4>{ { "RenderSystem", AccessTypes::READ_WRITE }, { "RenderOrchestrator", AccessTypes::READ_WRITE } };
		onTextureLoadHandle = initializeInfo.GameInstance->StoreDynamicTask("loadTexture", Task<TextureResourceManager*, TextureResourceManager::TextureInfo, TextureLoadInfo>::Create<RenderOrchestrator, &RenderOrchestrator::onTextureLoad>(this), taskDependencies);
	}

	{
		const auto taskDependencies = GTSL::Array<TaskDependency, 4>{ { "RenderOrchestrator", AccessTypes::READ } };
		onShaderInfosLoadHandle = initializeInfo.GameInstance->StoreDynamicTask("onShaderInfosLoaded", Task<MaterialResourceManager*, GTSL::Array<MaterialResourceManager::ShaderInfo, 8>, ShaderLoadInfo>::Create<RenderOrchestrator, &RenderOrchestrator::onShaderInfosLoaded>(this), taskDependencies);
	}

	{
		const auto taskDependencies = GTSL::Array<TaskDependency, 4>{ { "RenderSystem", AccessTypes::READ_WRITE }, { "RenderOrchestrator", AccessTypes::READ_WRITE } };
		onShadersLoadHandle = initializeInfo.GameInstance->StoreDynamicTask("onShadersLoaded", Task<MaterialResourceManager*, GTSL::Array<MaterialResourceManager::ShaderInfo, 8>, GTSL::Range<byte*>, ShaderLoadInfo>::Create<RenderOrchestrator, &RenderOrchestrator::onShadersLoaded>(this), taskDependencies);
	}

	{
		GTSL::Array<MaterialSystem::SubSetInfo, 10> subSetInfos;

		{ // TEXTURES
			MaterialSystem::SubSetInfo subSetInfo;
			subSetInfo.Type = MaterialSystem::SubSetType::READ_TEXTURES;
			subSetInfo.Count = 16;
			subSetInfo.Handle = &textureSubsetsHandle;
			subSetInfos.EmplaceBack(subSetInfo);
		}

		{ // IMAGES
			MaterialSystem::SubSetInfo subSetInfo;
			subSetInfo.Type = MaterialSystem::SubSetType::WRITE_TEXTURES;
			subSetInfo.Count = 16;
			subSetInfo.Handle = &imagesSubsetHandle;
			subSetInfos.EmplaceBack(subSetInfo);
		}

		if (BE::Application::Get()->GetOption("rayTracing"))
		{
			{ //TOP LEVEL AS
				MaterialSystem::SubSetInfo subSetInfo;
				subSetInfo.Type = MaterialSystem::SubSetType::ACCELERATION_STRUCTURE;
				subSetInfo.Handle = &topLevelAsHandle;
				subSetInfo.Count = 1;
				subSetInfos.EmplaceBack(subSetInfo);
			}
		}

		materialSystem->AddSetLayout(renderSystem, "GlobalData", Id(), subSetInfos);
		materialSystem->AddSet(renderSystem, "GlobalData", "GlobalData", subSetInfos);
	}
	
	{
		GTSL::Array<MaterialSystem::MemberInfo, 2> members;

		MaterialSystem::MemberInfo memberInfo;
		memberInfo.Handle = &globalDataHandle;
		memberInfo.Type = MaterialSystem::Member::DataType::UINT32;
		memberInfo.Count = 4;
		members.EmplaceBack(memberInfo);

		globalDataBuffer = materialSystem->CreateBuffer(renderSystem, members);
	}

	{
		GTSL::Array<MaterialSystem::MemberInfo, 2> members;

		MaterialSystem::MemberInfo memberInfo;
		memberInfo.Handle = &cameraMatricesHandle;
		memberInfo.Type = MaterialSystem::Member::DataType::MATRIX4;
		memberInfo.Count = 4;
		members.EmplaceBack(memberInfo);

		cameraDataBuffer = materialSystem->CreateBuffer(renderSystem, members);
	}

	if (BE::Application::Get()->GetOption("rayTracing"))
	{
		shaderGroupsByName.Initialize(16, GetPersistentAllocator());
		
		auto* materialResorceManager = BE::Application::Get()->GetResourceManager<MaterialResourceManager>("MaterialResourceManager");

		GTSL::Vector<RayTracingPipeline::Group, BE::TAR> groups(16, GetTransientAllocator());
		GTSL::Vector<Pipeline::ShaderInfo, BE::TAR> shaderInfos(16, GetTransientAllocator());
		GTSL::Vector<GTSL::Buffer<BE::TAR>, BE::TAR> shaderBuffers(16, GetTransientAllocator());

		for (uint32 i = 0; i < materialResorceManager->GetRayTraceShaderCount(); ++i)
		{
			uint32 bufferSize = 0;
			bufferSize = materialResorceManager->GetRayTraceShaderSize(materialResorceManager->GetRayTraceShaderHandle(i));
			shaderBuffers.EmplaceBack();
			shaderBuffers[i].Allocate(bufferSize, 8, GetTransientAllocator());

			shaderGroupsByName.Emplace(materialResorceManager->GetRayTraceShaderHandle(i), i);

			auto material = materialResorceManager->LoadRayTraceShaderSynchronous(materialResorceManager->GetRayTraceShaderHandle(i), GTSL::Range<byte*>(shaderBuffers[i].GetCapacity(), shaderBuffers[i].GetData())); //TODO: VIRTUAL BUFFER INTERFACE

			Pipeline::ShaderInfo shaderInfo;
			shaderInfo.Blob = GTSL::Range<const byte*>(material.BinarySize, shaderBuffers[i].GetData());
			shaderInfo.Type = ConvertShaderType(material.ShaderType);
			shaderInfos.EmplaceBack(shaderInfo);

			RayTracingPipeline::Group group{};

			group.GeneralShader = RayTracingPipeline::Group::SHADER_UNUSED; group.ClosestHitShader = RayTracingPipeline::Group::SHADER_UNUSED;
			group.AnyHitShader = RayTracingPipeline::Group::SHADER_UNUSED; group.IntersectionShader = RayTracingPipeline::Group::SHADER_UNUSED;

			switch (material.ShaderType)
			{
			case GAL::ShaderType::RAY_GEN: {
				group.ShaderGroup = GAL::VulkanShaderGroupType::GENERAL; group.GeneralShader = i;
				++shaderCounts[GAL::RAY_GEN_TABLE_INDEX];
				break;
			}
			case GAL::ShaderType::MISS: {
				//generalShader is the index of the ray generation,miss, or callable shader from VkRayTracingPipelineCreateInfoKHR::pStages
				//in the group if the shader group has type of VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, and VK_SHADER_UNUSED_KHR otherwise.
				group.ShaderGroup = GAL::VulkanShaderGroupType::GENERAL; group.GeneralShader = i;
				++shaderCounts[GAL::MISS_TABLE_INDEX];
				break;
			}
			case GAL::ShaderType::CALLABLE: {
				group.ShaderGroup = GAL::VulkanShaderGroupType::GENERAL; group.GeneralShader = i;
				++shaderCounts[GAL::CALLABLE_TABLE_INDEX];
				break;
			}
			case GAL::ShaderType::CLOSEST_HIT: {
				//closestHitShader is the optional index of the closest hit shader from VkRayTracingPipelineCreateInfoKHR::pStages in the group if the shader group
				//has type of VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR or VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR, and VK_SHADER_UNUSED_KHR otherwise.
				group.ShaderGroup = GAL::VulkanShaderGroupType::TRIANGLES; group.ClosestHitShader = i;
				++shaderCounts[GAL::HIT_TABLE_INDEX];
				break;
			}
			case GAL::ShaderType::ANY_HIT: {
				//anyHitShader is the optional index of the any-hit shader from VkRayTracingPipelineCreateInfoKHR::pStages in the group if the
				//shader group has type of VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR or VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
				//and VK_SHADER_UNUSED_KHR otherwise.
				group.ShaderGroup = GAL::VulkanShaderGroupType::TRIANGLES; group.AnyHitShader = i;
				++shaderCounts[GAL::HIT_TABLE_INDEX];
				break;
			}
			case GAL::ShaderType::INTERSECTION: {
				//intersectionShader is the index of the intersection shader from VkRayTracingPipelineCreateInfoKHR::pStages in the group if the shader group
				//has type of VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR, and VK_SHADER_UNUSED_KHR otherwise.
				group.ShaderGroup = GAL::VulkanShaderGroupType::PROCEDURAL; group.IntersectionShader = i;
				++shaderCounts[GAL::HIT_TABLE_INDEX];
				break;
			}

			default: BE_LOG_MESSAGE("Non raytracing shader found in raytracing material");
			}

			groups.EmplaceBack(group);
		}

		RayTracingPipeline::CreateInfo createInfo;
		createInfo.RenderDevice = renderSystem->GetRenderDevice();
		if constexpr (_DEBUG) {
			GTSL::StaticString<32> name("Ray Tracing Pipeline: "); createInfo.Name = name;
		}

		createInfo.MaxRecursionDepth = 3;
		createInfo.Stages = shaderInfos;

		createInfo.PipelineLayout = materialSystem->GetSetLayoutPipelineLayout(Id("GlobalData"));

		createInfo.Groups = groups;
		rayTracingPipeline.Initialize(createInfo);

		auto handleSize = renderSystem->GetShaderGroupHandleSize();
		auto alignedHandleSize = GTSL::Math::RoundUpByPowerOf2(handleSize, renderSystem->GetShaderGroupBaseAlignment());

		{
			GTSL::Array<MaterialSystem::MemberInfo, 32> memberInfos;
			memberInfos.PushBack(MaterialSystem::MemberInfo{ alignedHandleSize / 4/*uint32*/, MaterialSystem::Member::DataType::UINT32, &sbtMemberHandle, {} }); //shader handle
			memberInfos.PushBack(MaterialSystem::MemberInfo{ 1, MaterialSystem::Member::DataType::UINT32, &renderGroupPointerHandle, {} }); //raytracing render group buffer
			memberInfos.PushBack(MaterialSystem::MemberInfo{ 1, MaterialSystem::Member::DataType::UINT32, &materialDataPointerHandle, {} }); //material buffer

			materialSystem->CreateBuffer(renderSystem, memberInfos);
		}

		GTSL::Buffer<BE::TAR> handlesBuffer; handlesBuffer.Allocate(groups.GetLength() * alignedHandleSize, renderSystem->GetShaderGroupBaseAlignment(), GetTransientAllocator());

		entrySizes[GAL::RAY_GEN_TABLE_INDEX] = alignedHandleSize;
		entrySizes[GAL::HIT_TABLE_INDEX] = alignedHandleSize + 128;
		entrySizes[GAL::MISS_TABLE_INDEX] = alignedHandleSize;
		entrySizes[GAL::CALLABLE_TABLE_INDEX] = alignedHandleSize;

		rayTracingPipeline.GetShaderGroupHandles(renderSystem->GetRenderDevice(), 0, groups.GetLength(), GTSL::Range<byte*>(handlesBuffer.GetCapacity(), handlesBuffer.GetData()));

		MaterialSystem::BufferIterator iterator;
		materialSystem->UpdateIteratorMember(iterator, sbtMemberHandle);

		for (uint32 h = 0; h < groups.GetLength(); ++h)
		{
			for (uint8 t = 0; t < alignedHandleSize / 4; ++t) {
				materialSystem->UpdateIteratorMemberIndex(iterator, t);
				materialSystem->WriteMultiBuffer(iterator, reinterpret_cast<uint32*>(handlesBuffer.GetData() + h * handleSize) + t);
			}
		}

		for (uint8 f = 0; f < renderSystem->GetPipelinedFrames(); ++f) {
			materialSystem->UpdateSet2(topLevelAsHandle, 0, renderSystem->GetTopLevelAccelerationStructure(f), f);
		}
	}
}

void RenderOrchestrator::Shutdown(const ShutdownInfo& shutdownInfo)
{
}

void RenderOrchestrator::Setup(TaskInfo taskInfo)
{
	auto positionMatrices = taskInfo.GameInstance->GetSystem<CameraSystem>("CameraSystem")->GetPositionMatrices();
	auto rotationMatrices = taskInfo.GameInstance->GetSystem<CameraSystem>("CameraSystem")->GetRotationMatrices();
	auto fovs = taskInfo.GameInstance->GetSystem<CameraSystem>("CameraSystem")->GetFieldOfViews();

	GTSL::Matrix4 projectionMatrix;
	GTSL::Math::BuildPerspectiveMatrix(projectionMatrix, fovs[0], 16.f / 9.f, 0.5f, 1000.f);

	auto cameraPosition = positionMatrices[0];

	cameraPosition(0, 3) *= -1;
	cameraPosition(1, 3) *= -1;

	auto viewMatrix = rotationMatrices[0] * cameraPosition;
	auto matrix = projectionMatrix * viewMatrix;

	auto* materialSystem = taskInfo.GameInstance->GetSystem<MaterialSystem>("MaterialSystem");
	
	RenderManager::SetupInfo setupInfo;
	setupInfo.GameInstance = taskInfo.GameInstance;
	setupInfo.RenderSystem = taskInfo.GameInstance->GetSystem<RenderSystem>("RenderSystem");
	setupInfo.MaterialSystem = materialSystem;
	setupInfo.ProjectionMatrix = projectionMatrix;
	setupInfo.ViewMatrix = viewMatrix;
	setupInfo.RenderOrchestrator = this;
	GTSL::ForEach(renderManagers, [&](SystemHandle renderManager) { taskInfo.GameInstance->GetSystem<RenderManager>(renderManager)->Setup(setupInfo); });

	for (auto e : latestLoadedTextures) {
		for (auto b : pendingMaterialsPerTexture[e]) {
			auto& material = materials[b.MaterialIndex];
			auto& materialInstance = materialInstances[b.MaterialInstance];
			if (++materialInstance.Counter == materialInstance.Target) {
				setMaterialInstanceAsLoaded(b, materialInstance.Name);
				//taskInfo.GameInstance->DispatchEvent("MaterialSystem", GetOnMaterialInstanceLoadEventHandle(), GTSL::MoveRef(material.Name), GTSL::MoveRef(materialInstance.Name));
			}
		}
	}

	latestLoadedTextures.ResizeDown(0);
}

void RenderOrchestrator::Render(TaskInfo taskInfo)
{
	auto* renderSystem = taskInfo.GameInstance->GetSystem<RenderSystem>("RenderSystem");
	auto& commandBuffer = *renderSystem->GetCurrentCommandBuffer();
	uint8 currentFrame = renderSystem->GetCurrentFrame();
	auto* materialSystem = taskInfo.GameInstance->GetSystem<MaterialSystem>("MaterialSystem");

	{
		commandBuffer.BeginRegion(renderSystem->GetRenderDevice(), GTSL::StaticString<64>("Render"));
	}

	materialSystem->BindSet(renderSystem, commandBuffer, "GlobalData", PipelineType::RASTER);
	materialSystem->BindSet(renderSystem, commandBuffer, "GlobalData", PipelineType::COMPUTE);
	materialSystem->BindSet(renderSystem, commandBuffer, "GlobalData", PipelineType::RAY_TRACING);
	
	BindData(renderSystem, materialSystem, commandBuffer, materialSystem->GetBuffer(globalDataBuffer));
	BindData(renderSystem, materialSystem, commandBuffer, materialSystem->GetBuffer(cameraDataBuffer));
	
	{
		auto* cameraSystem = taskInfo.GameInstance->GetSystem<CameraSystem>("CameraSystem");

		GTSL::Matrix4 projectionMatrix;
		GTSL::Math::BuildPerspectiveMatrix(projectionMatrix, cameraSystem->GetFieldOfViews()[0], 16.f / 9.f, 0.5f, 1000.f);

		auto positionMatrix = cameraSystem->GetPositionMatrices()[0];
		positionMatrix(0, 3) *= -1;
		positionMatrix(1, 3) *= -1;

		auto viewMatrix = cameraSystem->GetRotationMatrices()[0] * positionMatrix;

		MaterialSystem::BufferIterator bufferIterator;
		materialSystem->UpdateIteratorMember(bufferIterator, cameraMatricesHandle);

		*materialSystem->GetMemberPointer<GTSL::Matrix4>(bufferIterator) = viewMatrix;
		materialSystem->UpdateIteratorMemberIndex(bufferIterator, 1);
		*materialSystem->GetMemberPointer<GTSL::Matrix4>(bufferIterator) = projectionMatrix;
		materialSystem->UpdateIteratorMemberIndex(bufferIterator, 2);
		*materialSystem->GetMemberPointer<GTSL::Matrix4>(bufferIterator) = GTSL::Math::Inverse(viewMatrix);
		materialSystem->UpdateIteratorMemberIndex(bufferIterator, 3);
		*materialSystem->GetMemberPointer<GTSL::Matrix4>(bufferIterator) = GTSL::Math::Inverse(projectionMatrix);
	}

	if (renderSystem->GetRenderExtent() == 0) { return; }
	
	for (uint8 renderPassIndex = 0; renderPassIndex < renderPasses.GetLength();)
	{
		Id renderPassId;
		RenderPassData* renderPass;

		auto beginRenderPass = [&]()
		{
			if constexpr (_DEBUG)
			{
				GTSL::StaticString<64> name("Render Pass: "); name += renderPassId.GetString();
				commandBuffer.BeginRegion(renderSystem->GetRenderDevice(), name);
			}
			
			switch(renderPass->PassType)
			{
				case PassType::RASTER: // Don't transition attachments as API render pass will handle transitions
				{
					for (auto& e : renderPass->WriteAttachments) {
						updateImage(attachments.At(e.Name), e.Layout, renderPass->PipelineStages, true);
					}

					for (auto& e : renderPass->ReadAttachments) {
						updateImage(attachments.At(e.Name), e.Layout, renderPass->PipelineStages, false);
					}

					renderState.PipelineType = PipelineType::RASTER;
					renderState.ShaderStages = ShaderStage::VERTEX | ShaderStage::FRAGMENT;
						
					break;
				}
				
				case PassType::COMPUTE:
				{
					renderState.PipelineType = PipelineType::COMPUTE;
					renderState.ShaderStages = ShaderStage::COMPUTE;
					transitionImages(commandBuffer, renderSystem, materialSystem, renderPassId);
					break;
				}

				case PassType::RAY_TRACING:
				{
					renderState.PipelineType = PipelineType::RAY_TRACING;
					renderState.ShaderStages = ShaderStage::RAY_GEN | ShaderStage::CLOSEST_HIT | ShaderStage::MISS | ShaderStage::INTERSECTION | ShaderStage::CALLABLE;
					transitionImages(commandBuffer, renderSystem, materialSystem, renderPassId);
					break;
				}
				
				default: break;
			}

			BindData(renderSystem, materialSystem, commandBuffer, materialSystem->GetBuffer(renderPass->BufferHandle));
		};

		auto canBeginRenderPass = [&]()
		{
			renderPassId = renderPasses[renderPassIndex];
			renderPass = &renderPassesMap[renderPassId];
			++renderPassIndex;
			return renderPass->Enabled;
		};
		
		auto endRenderPass = [&]()
		{
			PopData();
			
			if constexpr (_DEBUG)
			{
				commandBuffer.EndRegion(renderSystem->GetRenderDevice());
			}
		};
		
		if (canBeginRenderPass())
		{
			beginRenderPass();

			auto doRender = [&]() { if (renderPassesFunctions.Find(renderPassId)) { renderPassesFunctions.At(renderPassId)(this, taskInfo.GameInstance, renderSystem, materialSystem, commandBuffer, renderPassId); } };
			
			switch (renderPass->PassType)
			{
			case PassType::RASTER:
			{
				CommandBuffer::BeginRenderPassInfo beginRenderPassInfo;
				beginRenderPassInfo.RenderDevice = renderSystem->GetRenderDevice();
				beginRenderPassInfo.RenderPass = apiRenderPasses[renderPass->APIRenderPass].RenderPass;
				beginRenderPassInfo.Framebuffer = getFrameBuffer(renderPass->APIRenderPass);
				beginRenderPassInfo.RenderArea = renderSystem->GetRenderExtent();
					
				GTSL::Array<GTSL::RGBA, 8> clearValues;
				for (uint8 i = 0; i < renderPass->WriteAttachments.GetLength(); ++i) {
					const auto& attachment = attachments.At(renderPass->WriteAttachments[i].Name);
					clearValues.EmplaceBack(attachment.ClearColor);
				}
					
				beginRenderPassInfo.ClearValues = clearValues;
				commandBuffer.BeginRenderPass(beginRenderPassInfo);

				auto doRaster = [&]() {
					doRender();
				};

				doRaster();

				endRenderPass();
					
				for (uint8 subPassIndex = 0; subPassIndex < subPasses[renderPass->APIRenderPass].GetLength() - 1; ++subPassIndex) {
					commandBuffer.AdvanceSubPass(CommandBuffer::AdvanceSubpassInfo{});
					if (canBeginRenderPass()) { beginRenderPass(); doRaster(); endRenderPass(); }
				}
				
				commandBuffer.EndRenderPass(renderSystem->GetRenderDevice());

				break;
			}

			case PassType::COMPUTE:
			case PassType::RAY_TRACING:
			{
				doRender();
				endRenderPass();
					
				break;
			}
			}
		}
	}

	{
		{
			GTSL::Array<CommandBuffer::BarrierData, 2> barriers(1);
			barriers[0].SetTextureBarrier({ renderSystem->GetSwapchainTextures()[currentFrame], TextureLayout::UNDEFINED, TextureLayout::TRANSFER_DST, AccessFlags::TRANSFER_READ, AccessFlags::TRANSFER_WRITE, TextureType::COLOR });
			commandBuffer.AddPipelineBarrier(renderSystem->GetRenderDevice(), barriers, PipelineStage::TRANSFER, PipelineStage::TRANSFER, GetTransientAllocator());
		}

		{
			auto& attachment = attachments.At(resultAttachment);

			GTSL::Array<CommandBuffer::BarrierData, 2> barriers(1);
			barriers[0].SetTextureBarrier({ renderSystem->GetTexture(attachment.TextureHandle), attachment.Layout, TextureLayout::TRANSFER_SRC, accessFlagsFromStageAndAccessType(attachment.ConsumingStages, attachment.WriteAccess), AccessFlags::TRANSFER_READ, TextureType::COLOR });
			commandBuffer.AddPipelineBarrier(renderSystem->GetRenderDevice(), barriers, attachment.ConsumingStages, PipelineStage::TRANSFER, GetTransientAllocator());

			updateImage(attachment, TextureLayout::TRANSFER_SRC, PipelineStage::TRANSFER, false);
		}

			
		commandBuffer.CopyTextureToTexture(renderSystem->GetRenderDevice(), renderSystem->GetTexture(attachments.At(resultAttachment).TextureHandle),
		renderSystem->GetSwapchainTextures()[currentFrame], TextureLayout::TRANSFER_SRC, TextureLayout::TRANSFER_DST,
			GTSL::Extent3D(renderSystem->GetRenderExtent()));

		{
			GTSL::Array<CommandBuffer::BarrierData, 2> barriers;
			barriers.EmplaceBack(CommandBuffer::TextureBarrier{ renderSystem->GetSwapchainTextures()[currentFrame], TextureLayout::TRANSFER_DST, TextureLayout::PRESENTATION, AccessFlags::TRANSFER_READ, AccessFlags::TRANSFER_WRITE, TextureType::COLOR });
			commandBuffer.AddPipelineBarrier(renderSystem->GetRenderDevice(), barriers, PipelineStage::TRANSFER, PipelineStage::TRANSFER, GetTransientAllocator());
		}
	}
	
	commandBuffer.EndRegion(renderSystem->GetRenderDevice());

	PopData();
	PopData();
}

void RenderOrchestrator::AddRenderManager(GameInstance* gameInstance, const Id renderManager, const SystemHandle systemReference)
{
	systems.EmplaceBack(renderManager);
	
	gameInstance->RemoveTask(SETUP_TASK_NAME, "GameplayEnd");
	gameInstance->RemoveTask(RENDER_TASK_NAME, "RenderDo");

	GTSL::Array<TaskDependency, 32> dependencies(systems.GetLength());
	{
		for (uint32 i = 0; i < dependencies.GetLength(); ++i)
		{
			dependencies[i].AccessedObject = systems[i];
			dependencies[i].Access = AccessTypes::READ;
		}
	}

	dependencies.EmplaceBack("RenderSystem", AccessTypes::READ);
	dependencies.EmplaceBack("MaterialSystem", AccessTypes::READ);

	{
		GTSL::Array<TaskDependency, 32> managerDependencies;

		managerDependencies.PushBack(dependencies);
		
		GTSL::Array<TaskDependency, 16> managerSetupDependencies;

		gameInstance->GetSystem<RenderManager>(systemReference)->GetSetupAccesses(managerSetupDependencies);

		managerDependencies.PushBack(managerSetupDependencies);
		
		setupSystemsAccesses.PushBack(managerDependencies);
	}
	

	gameInstance->AddTask(SETUP_TASK_NAME, GTSL::Delegate<void(TaskInfo)>::Create<RenderOrchestrator, &RenderOrchestrator::Setup>(this), dependencies, "GameplayEnd", "RenderStart");
	gameInstance->AddTask(RENDER_TASK_NAME, GTSL::Delegate<void(TaskInfo)>::Create<RenderOrchestrator, &RenderOrchestrator::Render>(this), dependencies, "RenderDo", "RenderFinished");
	renderManagers.Emplace(renderManager, systemReference);
}

void RenderOrchestrator::RemoveRenderManager(GameInstance* gameInstance, const Id renderGroupName, const SystemHandle systemReference)
{
	const auto element = systems.Find(renderGroupName);
	BE_ASSERT(element.State())
	
	systems.Pop(element.Get());
	
	setupSystemsAccesses.Pop(element.Get());
	gameInstance->RemoveTask(SETUP_TASK_NAME, "GameplayEnd");
	gameInstance->RemoveTask(RENDER_TASK_NAME, "RenderDo");

	GTSL::Array<TaskDependency, 32> dependencies(systems.GetLength());
	{
		for(uint32 i = 0; i < dependencies.GetLength(); ++i)
		{
			dependencies[i].AccessedObject = systems[i];
			dependencies[i].Access = AccessTypes::READ;
		}
	}

	dependencies.EmplaceBack("RenderSystem", AccessTypes::READ);
	dependencies.EmplaceBack("MaterialSystem", AccessTypes::READ);
	
	gameInstance->AddTask(SETUP_TASK_NAME, GTSL::Delegate<void(TaskInfo)>::Create<RenderOrchestrator, &RenderOrchestrator::Setup>(this), dependencies, "GameplayEnd", "RenderStart");
	gameInstance->AddTask(RENDER_TASK_NAME, GTSL::Delegate<void(TaskInfo)>::Create<RenderOrchestrator, &RenderOrchestrator::Render>(this), dependencies, "RenderDo", "RenderFinished");
}

MaterialInstanceHandle RenderOrchestrator::CreateMaterial(const CreateMaterialInfo& info)
{
	uint32 material_size = 0;
	info.MaterialResourceManager->GetMaterialSize(info.MaterialName, material_size);

	auto materialIndex = materials.Emplace();

	GTSL::Buffer<BE::PAR> material_buffer; material_buffer.Allocate(material_size, 32, GetPersistentAllocator());

	const auto acts_on = GTSL::Array<TaskDependency, 16>{ { "RenderSystem", AccessTypes::READ_WRITE }, { "MaterialSystem", AccessTypes::READ_WRITE }, { "RenderOrchestrator", AccessTypes::READ_WRITE } };
	MaterialResourceManager::MaterialLoadInfo material_load_info;
	material_load_info.ActsOn = acts_on;
	material_load_info.GameInstance = info.GameInstance;
	material_load_info.Name = info.MaterialName;
	material_load_info.DataBuffer = GTSL::Range<byte*>(material_buffer.GetCapacity(), material_buffer.GetData());
	auto* matLoadInfo = GTSL::New<MaterialLoadInfo>(GetPersistentAllocator(), info.RenderSystem, MoveRef(material_buffer), materialIndex, 0, info.TextureResourceManager);
	material_load_info.UserData = DYNAMIC_TYPE(MaterialLoadInfo, matLoadInfo);
	material_load_info.OnMaterialLoad = Task<MaterialResourceManager::OnMaterialLoadInfo>::Create<RenderOrchestrator, &RenderOrchestrator::onMaterialLoaded>(this);
	info.MaterialResourceManager->LoadMaterial(material_load_info);

	return info.MaterialName;
}

void RenderOrchestrator::AddAttachment(Id name, uint8 bitDepth, uint8 componentCount, GAL::ComponentType compType, TextureType::value_type type, GTSL::RGBA clearColor)
{
	Attachment attachment;
	attachment.Name = name;
	attachment.Type = type;
	attachment.Uses = 0;
	
	GAL::FormatDescriptor formatDescriptor;
	
	if (type & TextureType::COLOR)
	{		
		formatDescriptor = GAL::FormatDescriptor(compType, componentCount, bitDepth, GAL::TextureType::COLOR, 0, 1, 2, 3);
		attachment.Uses |= TextureUse::STORAGE;
		attachment.Uses |= TextureUse::COLOR_ATTACHMENT;
		attachment.Uses |= TextureUse::TRANSFER_SOURCE;
	}
	else
	{
		formatDescriptor = GAL::FormatDescriptor(compType, componentCount, bitDepth, GAL::TextureType::DEPTH, 0, 0, 0, 0);
		
		attachment.Uses |= TextureUse::DEPTH_STENCIL_ATTACHMENT;
	}
	
	attachment.FormatDescriptor = formatDescriptor;

	attachment.Uses |= TextureUse::SAMPLE;

	attachment.ClearColor = clearColor;
	attachment.Layout = TextureLayout::UNDEFINED;
	attachment.WriteAccess = false;
	attachment.ConsumingStages = PipelineStage::TOP_OF_PIPE;

	attachments.Emplace(name, attachment);
}

void RenderOrchestrator::AddPass(RenderSystem* renderSystem, MaterialSystem* materialSystem, GTSL::Range<const PassData*> passesData)
{
	GTSL::Array<Id, 16> frameUsedAttachments;

	GTSL::Array<GTSL::StaticMap<Id, uint32, 16>, 16> attachmentReadsPerPass;
	
	for (uint8 passIndex = 0; passIndex < passesData.ElementCount(); ++passIndex) {
		auto addIfNotUsed = [&](const Id name) {
			for (auto e : frameUsedAttachments) { if (e == name) { return; } }
			frameUsedAttachments.EmplaceBack(name);
		};
		
		attachmentReadsPerPass.EmplaceBack();
		
		for(auto e : passesData[passIndex].ReadAttachments) { addIfNotUsed(e.Name); }
		for(auto e : passesData[passIndex].WriteAttachments) { addIfNotUsed(e.Name); }

		resultAttachment = passesData[passIndex].ResultAttachment;
	}

	for (uint8 passIndex = 0; passIndex < passesData.ElementCount(); ++passIndex) {
		for (auto e : frameUsedAttachments)
		{
			uint32 pass = 0;

			for (uint8 i = passIndex; i < passesData.ElementCount(); ++i) {
				for (auto r : passesData[i].ReadAttachments) { if (e == r.Name) { pass = i; } }
			}

			attachmentReadsPerPass[passIndex].Emplace(e, pass);
		}
		
		attachmentReadsPerPass[passIndex].At(resultAttachment) = 0xFFFFFFFF; //set result attachment last read as "infinte" so it will always be stored
	}

	{
		auto& finalAttachment = attachments.At(resultAttachment);

		finalAttachment.FormatDescriptor = GAL::FORMATS::BGRA_I8;
	}
	
	for (uint8 passIndex = 0; passIndex < passesData.ElementCount(); ++passIndex)
	{		
		switch (passesData[passIndex].PassType)
		{
		case PassType::RASTER:
		{
			uint32 contiguousRasterPassCount = passIndex;
			while (contiguousRasterPassCount < passesData.ElementCount() && passesData[contiguousRasterPassCount].PassType == PassType::RASTER) {
				++contiguousRasterPassCount;
			}
				
			uint32 lastContiguousRasterPassIndex = contiguousRasterPassCount - 1;
				
			auto& apiRenderPassData = apiRenderPasses[apiRenderPasses.EmplaceBack()];
				
			RenderPass::CreateInfo renderPassCreateInfo;
			renderPassCreateInfo.RenderDevice = renderSystem->GetRenderDevice();
			if constexpr (_DEBUG) {
				auto name = GTSL::StaticString<32>("RenderPass");
				renderPassCreateInfo.Name = name;
			}

			GTSL::Array<Id, 16> renderPassUsedAttachments;
			GTSL::Array<GTSL::Array<Id, 16>, 16> usedAttachmentsPerSubPass;

			for (uint8 p = passIndex, s = 0; p < contiguousRasterPassCount; ++p, ++s) {
				usedAttachmentsPerSubPass.EmplaceBack();
				
				for (auto& ra : passesData[p].ReadAttachments)
				{
					if (!renderPassUsedAttachments.Find(ra.Name).State()) { renderPassUsedAttachments.EmplaceBack(ra.Name); }
					if (!usedAttachmentsPerSubPass[s].Find(ra.Name).State()) { usedAttachmentsPerSubPass[s].EmplaceBack(ra.Name); }
				}
				
				for (auto& wa : passesData[p].WriteAttachments)
				{
					if (!renderPassUsedAttachments.Find(wa.Name).State()) { renderPassUsedAttachments.EmplaceBack(wa.Name); }
					if (!usedAttachmentsPerSubPass[s].Find(wa.Name).State()) { usedAttachmentsPerSubPass[s].EmplaceBack(wa.Name); }
				}
			}

			GTSL::Array<RenderPass::AttachmentDescriptor, 16> attachmentDescriptors;

			for (auto e : renderPassUsedAttachments)
			{
				auto& attachment = attachments.At(e);

				RenderPass::AttachmentDescriptor attachmentDescriptor;
				attachmentDescriptor.Format = static_cast<TextureFormat>(GAL::FormatToVkFomat(GAL::MakeFormatFromFormatDescriptor(attachment.FormatDescriptor)));
				attachmentDescriptor.LoadOperation = GAL::RenderTargetLoadOperations::CLEAR;
				if(attachmentReadsPerPass[lastContiguousRasterPassIndex].At(e) > lastContiguousRasterPassIndex) {
					attachmentDescriptor.StoreOperation = GAL::RenderTargetStoreOperations::STORE;
				}
				else {
					attachmentDescriptor.StoreOperation = GAL::RenderTargetStoreOperations::UNDEFINED;
				}
				attachmentDescriptor.InitialLayout = TextureLayout::UNDEFINED;
				attachmentDescriptor.FinalLayout = attachment.Type & TextureType::COLOR ? TextureLayout::COLOR_ATTACHMENT : TextureLayout::DEPTH_STENCIL_ATTACHMENT; //TODO: SELECT CORRECT END LAYOUT
				attachmentDescriptors.EmplaceBack(attachmentDescriptor);
			}

			renderPassCreateInfo.RenderPassAttachments = attachmentDescriptors;

			GTSL::Array<RenderPass::SubPassDescriptor, 8> subPassDescriptors;
			GTSL::Array<GTSL::Array<RenderPass::AttachmentReference, 8>, 8> readAttachmentReferences(contiguousRasterPassCount);
			GTSL::Array<GTSL::Array<RenderPass::AttachmentReference, 8>, 8> writeAttachmentReferences(contiguousRasterPassCount);
			GTSL::Array<GTSL::Array<uint8, 8>, 8> preserveAttachmentReferences(contiguousRasterPassCount);

			AccessFlags::value_type sourceAccessFlags = 0, destinationAccessFlags = 0;
			PipelineStage::value_type sourcePipelineStages = PipelineStage::TOP_OF_PIPE, destinationPipelineStages = PipelineStage::TOP_OF_PIPE;
				
			subPasses.EmplaceBack();

			for (uint32 s = 0; s < contiguousRasterPassCount; ++s, ++passIndex)
			{
				auto& renderPass = renderPassesMap.Emplace(passesData[passIndex].Name);
				renderPasses.EmplaceBack(passesData[passIndex].Name);
				renderPass.APIRenderPass = apiRenderPasses.GetLength() - 1;

				renderPass.PassType = PassType::RASTER;
				renderPass.PipelineStages = PipelineStage::COLOR_ATTACHMENT_OUTPUT;

				RenderPass::SubPassDescriptor subPassDescriptor;

				auto getAttachmentIndex = [&](const Id name)
				{
					auto res = renderPassUsedAttachments.Find(name); return res.State() ? res.Get() : GAL::ATTACHMENT_UNUSED;
				};

				subPassDescriptor.DepthAttachmentReference.Layout = TextureLayout::DEPTH_ATTACHMENT;
				subPassDescriptor.DepthAttachmentReference.Index = GAL::ATTACHMENT_UNUSED;
				
				for (auto& e : passesData[passIndex].ReadAttachments)
				{
					if (attachments.At(e.Name).Type & TextureType::COLOR)
					{
						RenderPass::AttachmentReference attachmentReference;
						attachmentReference.Layout = TextureLayout::SHADER_READ_ONLY;
						attachmentReference.Index = getAttachmentIndex(e.Name);

						readAttachmentReferences[s].EmplaceBack(attachmentReference);

						renderPass.ReadAttachments.EmplaceBack(AttachmentData{ e.Name, TextureLayout::SHADER_READ_ONLY, PipelineStage::TOP_OF_PIPE });
						destinationAccessFlags |= AccessFlags::COLOR_ATTACHMENT_READ;
						destinationPipelineStages |= PipelineStage::COLOR_ATTACHMENT_OUTPUT;
					}
					else
					{
						destinationAccessFlags |= AccessFlags::DEPTH_STENCIL_ATTACHMENT_READ;
						destinationPipelineStages |= PipelineStage::EARLY_FRAGMENT_TESTS | PipelineStage::LATE_FRAGMENT_TESTS;
						
						subPassDescriptor.DepthAttachmentReference.Layout = TextureLayout::DEPTH_STENCIL_ATTACHMENT;
						subPassDescriptor.DepthAttachmentReference.Index = getAttachmentIndex(e.Name);
						renderPass.WriteAttachments.EmplaceBack(AttachmentData{ e.Name, TextureLayout::DEPTH_STENCIL_ATTACHMENT, PipelineStage::EARLY_FRAGMENT_TESTS });
					}
				}

				subPassDescriptor.ReadColorAttachments = readAttachmentReferences[s];

				for (auto e : passesData[passIndex].WriteAttachments)
				{
					if (attachments.At(e.Name).Type & TextureType::COLOR)
					{
						RenderPass::AttachmentReference attachmentReference;
						attachmentReference.Layout = TextureLayout::COLOR_ATTACHMENT;
						attachmentReference.Index = getAttachmentIndex(e.Name);

						writeAttachmentReferences[s].EmplaceBack(attachmentReference);

						renderPass.WriteAttachments.EmplaceBack(AttachmentData{ e.Name, TextureLayout::COLOR_ATTACHMENT, PipelineStage::COLOR_ATTACHMENT_OUTPUT });
						destinationAccessFlags |= AccessFlags::COLOR_ATTACHMENT_WRITE;
						destinationPipelineStages |= PipelineStage::COLOR_ATTACHMENT_OUTPUT;
					}
					else
					{

						destinationAccessFlags |= AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE;
						destinationPipelineStages |= PipelineStage::EARLY_FRAGMENT_TESTS | PipelineStage::LATE_FRAGMENT_TESTS;
						
						subPassDescriptor.DepthAttachmentReference.Layout = TextureLayout::DEPTH_STENCIL_ATTACHMENT;
						subPassDescriptor.DepthAttachmentReference.Index = getAttachmentIndex(e.Name);
						renderPass.WriteAttachments.EmplaceBack(AttachmentData{ e.Name, TextureLayout::DEPTH_STENCIL_ATTACHMENT, PipelineStage::EARLY_FRAGMENT_TESTS });
					}
				}

				subPassDescriptor.WriteColorAttachments = writeAttachmentReferences[s];

				{
					for (auto b : renderPassUsedAttachments)
					{
						if (!usedAttachmentsPerSubPass[s].Find(b).State()) // If attachment is not used this sub pass
						{
							if (attachmentReadsPerPass[s].At(b) > s) // And attachment is read after this pass
							{
								preserveAttachmentReferences[s].EmplaceBack(getAttachmentIndex(b));
							}
						}
					}
				}
				
				subPassDescriptor.PreserveAttachments = preserveAttachmentReferences[s];

				subPassDescriptors.EmplaceBack(subPassDescriptor);

				subPasses.back().EmplaceBack();
				auto& newSubPass = subPasses.back().back();
				newSubPass.Name = passesData[passIndex].Name;

				sourceAccessFlags = destinationAccessFlags;
				sourcePipelineStages = destinationPipelineStages;
			}

			--passIndex;

			renderPassCreateInfo.SubPasses = subPassDescriptors;

			GTSL::Array<RenderPass::SubPassDependency, 16> subPassDependencies;

			for (uint8 i = 0; i < subPasses.back().GetLength() / 2; ++i)
			{
				RenderPass::SubPassDependency e;
				e.SourcePipelineStage = sourcePipelineStages;
				e.DestinationPipelineStage = destinationPipelineStages;
				
				e.SourceSubPass = i;
				e.DestinationSubPass = i + 1;
				e.SourceAccessFlags = sourceAccessFlags;
				e.DestinationAccessFlags = destinationAccessFlags;

				subPassDependencies.EmplaceBack(e);
			}

			renderPassCreateInfo.SubPassDependencies = subPassDependencies;

			apiRenderPassData.UsedAttachments = renderPassUsedAttachments;
				
			apiRenderPassData.RenderPass = RenderPass(renderPassCreateInfo);

			break;
		}
		case PassType::COMPUTE:
		{
			renderPasses.EmplaceBack(passesData[passIndex].Name);
			auto& renderPass = renderPassesMap.Emplace(passesData[passIndex].Name);

			renderPass.PassType = PassType::COMPUTE;
			renderPass.PipelineStages = PipelineStage::COMPUTE_SHADER;

			for (auto& e : passesData[passIndex].WriteAttachments) {
				AttachmentData attachmentData;
				attachmentData.Name = e.Name;
				attachmentData.Layout = TextureLayout::GENERAL;
				attachmentData.ConsumingStages = PipelineStage::COMPUTE_SHADER;
				renderPass.WriteAttachments.EmplaceBack(attachmentData);
			}

			for (auto& e : passesData[passIndex].ReadAttachments) {
				AttachmentData attachmentData;
				attachmentData.Name = e.Name;
				attachmentData.Layout = TextureLayout::SHADER_READ_ONLY;
				attachmentData.ConsumingStages = PipelineStage::COMPUTE_SHADER;
				renderPass.ReadAttachments.EmplaceBack(attachmentData);
			}

			break;
		}
		case PassType::RAY_TRACING:
		{
			renderPasses.EmplaceBack(passesData[passIndex].Name);
			auto& renderPass = renderPassesMap.Emplace(passesData[passIndex].Name);

			renderPass.PassType = PassType::RAY_TRACING;
			renderPass.PipelineStages = PipelineStage::RAY_TRACING_SHADER;

			for (auto& e : passesData[passIndex].WriteAttachments) {
				AttachmentData attachmentData;
				attachmentData.Name = e.Name;
				attachmentData.Layout = TextureLayout::GENERAL;
				attachmentData.ConsumingStages = PipelineStage::RAY_TRACING_SHADER;
				renderPass.WriteAttachments.EmplaceBack(attachmentData);
			}

			for (auto& e : passesData[passIndex].ReadAttachments) {
				AttachmentData attachmentData;
				attachmentData.Name = e.Name;
				attachmentData.Layout = TextureLayout::SHADER_READ_ONLY;
				attachmentData.ConsumingStages = PipelineStage::RAY_TRACING_SHADER;
				renderPass.ReadAttachments.EmplaceBack(attachmentData);
			}

			break;
		}
		}
	}
	
	for (uint8 rp = 0; rp < renderPasses.GetLength(); ++rp)
	{
		auto& renderPass = renderPassesMap.At(renderPasses[rp]);
		
		{
			//GTSL::Array<MaterialSystem::SubSetInfo, 8> subSets;
			//subSets.EmplaceBack(MaterialSystem::SubSetInfo{ MaterialSystem::SubSetType::READ_TEXTURES, &renderPass.ReadAttachmentsHandle, 16 });
			//subSets.EmplaceBack(MaterialSystem::SubSetInfo{ MaterialSystem::SubSetType::WRITE_TEXTURES, &renderPass.WriteAttachmentsHandle, 16 });
			//renderPass.AttachmentsSetHandle = materialSystem->AddSet(renderSystem, renderPasses[rp], "RenderPasses", subSets);

			GTSL::Array<MaterialSystem::MemberInfo, 2> members;

			MaterialSystem::MemberInfo memberInfo;
			memberInfo.Handle = &renderPass.AttachmentsIndicesHandle;
			memberInfo.Type = MaterialSystem::Member::DataType::UINT32;
			memberInfo.Count = 16; //TODO: MAKE NUMBER OF MEMBERS
			members.EmplaceBack(memberInfo);
			
			renderPass.BufferHandle = materialSystem->CreateBuffer(renderSystem, members);
		}
	}
}

void RenderOrchestrator::OnResize(RenderSystem* renderSystem, MaterialSystem* materialSystem, const GTSL::Extent2D newSize)
{	
	auto resize = [&](Attachment& attachment) -> void
	{
		if(attachment.TextureHandle) {
			attachment.TextureHandle = renderSystem->CreateTexture(attachment.FormatDescriptor, newSize, attachment.Uses, false);
		}
		else {
			attachment.TextureHandle = renderSystem->CreateTexture(attachment.FormatDescriptor, newSize, attachment.Uses, false);
			attachment.ImageIndex = imageIndex++;
		}

		if(attachment.FormatDescriptor.Type == GAL::TextureType::COLOR)
		{
			materialSystem->WriteSetTexture(renderSystem, imagesSubsetHandle, attachment.TextureHandle, attachment.ImageIndex);
		}
	};

	renderSystem->Wait();

	GTSL::ForEach(attachments, resize);

	for (auto& apiRenderPassData : apiRenderPasses)
	{
		if (apiRenderPassData.FrameBuffer.GetHandle())
		{
			apiRenderPassData.FrameBuffer.Destroy(renderSystem->GetRenderDevice());
		}

		FrameBuffer::CreateInfo framebufferCreateInfo;
		framebufferCreateInfo.RenderDevice = renderSystem->GetRenderDevice();
		if constexpr (_DEBUG) { framebufferCreateInfo.Name = GTSL::StaticString<32>("FrameBuffer"); }

		GTSL::Array<TextureView, 16> textureViews;
		for (auto e : apiRenderPassData.UsedAttachments) { textureViews.EmplaceBack(renderSystem->GetTextureView(attachments.At(e).TextureHandle)); }

		framebufferCreateInfo.TextureViews = textureViews;
		framebufferCreateInfo.RenderPass = &apiRenderPassData.RenderPass;
		framebufferCreateInfo.Extent = renderSystem->GetRenderExtent();

		apiRenderPassData.FrameBuffer = FrameBuffer(framebufferCreateInfo);
	}

	for (uint8 rp = 0; rp < renderPasses.GetLength(); ++rp)
	{
		auto& renderPass = renderPassesMap.At(renderPasses[rp]);

		MaterialSystem::BufferIterator bufferIterator; uint8 attachmentIndex = 0;

		materialSystem->UpdateIteratorMember(bufferIterator, renderPass.AttachmentsIndicesHandle);
		
		for (uint8 r = 0; r < renderPass.ReadAttachments.GetLength(); ++r)
		{
			auto& attachment = attachments.At(renderPass.ReadAttachments[r].Name);
			auto name = attachment.Name;

			materialSystem->WriteMultiBuffer(bufferIterator, &attachment.ImageIndex);
			
			materialSystem->UpdateIteratorMemberIndex(bufferIterator, ++attachmentIndex);
		}
		
		for (uint8 w = 0; w < renderPass.WriteAttachments.GetLength(); ++w)
		{
			auto& attachment = attachments.At(renderPass.WriteAttachments[w].Name);
			auto name = attachment.Name;

			if (attachment.Type & TextureType::COLOR) {
				materialSystem->WriteMultiBuffer(bufferIterator, &attachment.ImageIndex);
				materialSystem->UpdateIteratorMemberIndex(bufferIterator, ++attachmentIndex);
			}
		}
	}
}

void RenderOrchestrator::ToggleRenderPass(Id renderPassName, bool enable)
{
	auto& renderPass = renderPassesMap[renderPassName];
	switch (renderPass.PassType)
	{
		case PassType::RASTER: break;
		case PassType::COMPUTE: break;
		case PassType::RAY_TRACING: enable = enable && BE::Application::Get()->GetOption("rayTracing"); break; // Enable render pass only if function is enaled in settings
		default: break;
	}
	
	renderPass.Enabled = enable;
}

void RenderOrchestrator::AddToRenderPass(Id renderPass, Id renderGroup)
{
	if (renderPassesMap.Find(renderPass))
	{
		renderPassesMap.At(renderPass).RenderGroups.EmplaceBack(renderGroup);
	}
}

void RenderOrchestrator::AddMesh(const RenderSystem::MeshHandle meshHandle, const MaterialInstanceHandle materialHandle, const uint32 instanceIndex)
{
	auto result = loadedMaterialInstances.TryGet(materialHandle);

	if (result.State()) [[likely]]
	{
		materialInstances[result.Get()].Meshes.EmplaceBack(meshHandle, 1, instanceIndex);
	}
	else
	{
		auto awaitingResult = awaitingMaterialInstances.TryEmplace(materialHandle);

		if (awaitingResult.State())
		{
			awaitingResult.Get().Meshes.Initialize(8, GetPersistentAllocator());
		}

		awaitingResult.Get().Meshes.EmplaceBack(meshHandle, 1, instanceIndex);
	}
}

void RenderOrchestrator::UpdateIndexStream(IndexStreamHandle indexStreamHandle, CommandBuffer commandBuffer, RenderSystem* renderSystem, MaterialSystem* materialSystem)
{
	commandBuffer.UpdatePushConstant(renderSystem->GetRenderDevice(), renderState.PipelineLayout, 64ull + (indexStreamHandle() * 4),
		GTSL::Range<const byte*>(4, reinterpret_cast<const byte*>(&renderState.IndexStreams[indexStreamHandle()])), ShaderStage::ALL);

	++renderState.IndexStreams[indexStreamHandle()];
}

void RenderOrchestrator::UpdateIndexStream(IndexStreamHandle indexStreamHandle, CommandBuffer commandBuffer, RenderSystem* renderSystem, MaterialSystem* materialSystem, uint32 value)
{
	renderState.IndexStreams[indexStreamHandle()] = value;
	
	commandBuffer.UpdatePushConstant(renderSystem->GetRenderDevice(), renderState.PipelineLayout, 64ull + (indexStreamHandle() * 4),
		GTSL::Range<const byte*>(4, reinterpret_cast<const byte*>(&renderState.IndexStreams[indexStreamHandle()])), ShaderStage::ALL);
}

void RenderOrchestrator::BindData(const RenderSystem* renderSystem, const MaterialSystem* materialSystem, CommandBuffer commandBuffer, Buffer buffer)
{
	auto bufferAddress = buffer.GetAddress(renderSystem->GetRenderDevice());
	
	BE_ASSERT((bufferAddress % 16ull) == 0, "Non divisible");

	BE_ASSERT((bufferAddress / 16ull) < 0xFFFFFFFF, "Bigger than uint32!");
	
	uint32 dividedBufferAddress = bufferAddress / 16;

	renderState.PipelineLayout = materialSystem->GetSetLayoutPipelineLayout("GlobalData");

	commandBuffer.UpdatePushConstant(renderSystem->GetRenderDevice(), renderState.PipelineLayout, renderState.Offset,
		GTSL::Range<const byte*>(4, reinterpret_cast<const byte*>(&dividedBufferAddress)), ShaderStage::ALL);

	renderState.Offset += 4;
}

void RenderOrchestrator::BindMaterial(RenderSystem* renderSystem, CommandBuffer commandBuffer,
	MaterialHandle materialHandle)
{
	commandBuffer.BindPipeline(renderSystem->GetRenderDevice(), materials[loadedMaterials[materialHandle]].Pipeline, PipelineType::RASTER);
}

AccessFlags::value_type RenderOrchestrator::accessFlagsFromStageAndAccessType(PipelineStage::value_type stage, bool writeAccess)
{
	AccessFlags::value_type accessFlags = 0; //TODO: SWITCH FLAGS BY ATTACHMENT TYPE. E.J: COLOR, DEPTH, etc
	accessFlags |= stage & PipelineStage::COLOR_ATTACHMENT_OUTPUT ? writeAccess ? AccessFlags::COLOR_ATTACHMENT_WRITE : AccessFlags::COLOR_ATTACHMENT_READ : 0;
	accessFlags |= stage & PipelineStage::EARLY_FRAGMENT_TESTS ? writeAccess ? AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE : AccessFlags::DEPTH_STENCIL_ATTACHMENT_READ : 0;
	accessFlags |= stage & PipelineStage::LATE_FRAGMENT_TESTS ? writeAccess ? AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE : AccessFlags::DEPTH_STENCIL_ATTACHMENT_READ : 0;
	accessFlags |= stage & (PipelineStage::RAY_TRACING_SHADER | PipelineStage::COMPUTE_SHADER) ? writeAccess ? AccessFlags::SHADER_WRITE : AccessFlags::SHADER_READ : 0;
	accessFlags |= stage & PipelineStage::TRANSFER ? writeAccess ? AccessFlags::TRANSFER_WRITE : AccessFlags::TRANSFER_READ : 0;
	return accessFlags;
}

void RenderOrchestrator::renderScene(GameInstance*, RenderSystem* renderSystem, MaterialSystem* materialSystem, CommandBuffer commandBuffer, Id rp)
{	
	for (auto rg : renderPassesMap.At(rp).RenderGroups)
	{
		auto renderGroupIndexStream = AddIndexStream();
		BindData(renderSystem, materialSystem, commandBuffer, materialSystem->GetBuffer(rg));

		auto forEachMaterial = [&](const uint32 materialIndex)
		{
			const auto& materialData = materials[materialIndex];
			
			BindMaterial(renderSystem, commandBuffer, materialData.Name);
			BindData(renderSystem, materialSystem, commandBuffer, materialSystem->GetBuffer(materialData.Name));
			auto materialInstanceIndexStream = AddIndexStream();

			for (auto b : materialData.MaterialInstances)
			{
				//auto& materialInstance = loadedMaterialInstances[b()];
				const auto& meshes = materialInstances[b.First].Meshes;
				UpdateIndexStream(materialInstanceIndexStream, commandBuffer, renderSystem, materialSystem, b.Second);

				for (auto meshHandle : meshes)
				{
					UpdateIndexStream(renderGroupIndexStream, commandBuffer, renderSystem, materialSystem, meshHandle.InstanceIndex);
					renderSystem->RenderMesh(meshHandle.Handle, meshHandle.InstanceCount);
				}
			}

			PopIndexStream(materialInstanceIndexStream);
			PopData();
		};
		GTSL::ForEach(loadedMaterials, forEachMaterial);

		PopData();
		PopIndexStream(renderGroupIndexStream);
	}
}

void RenderOrchestrator::renderUI(GameInstance* gameInstance, RenderSystem* renderSystem, MaterialSystem* materialSystem, CommandBuffer commandBuffer, Id rp)
{
	auto* uiRenderManager = gameInstance->GetSystem<UIRenderManager>("UIRenderManager");

	//materialSystem->BindSet(renderSystem, commandBuffer, Id("UIRenderGroup"));

	auto* uiSystem = gameInstance->GetSystem<UIManager>("UIManager");
	auto* canvasSystem = gameInstance->GetSystem<CanvasSystem>("CanvasSystem");

	auto canvases = uiSystem->GetCanvases();

	for (auto& ref : canvases)
	{
		auto& canvas = canvasSystem->GetCanvas(ref);
		auto canvasSize = canvas.GetExtent();

		auto& organizers = canvas.GetOrganizersTree();

		auto primitives = canvas.GetPrimitives();
		auto squares = canvas.GetSquares();

		const auto* parentOrganizer = organizers[0];

		uint32 squareIndex = 0;
		for (auto& e : squares)
		{
			auto mat = primitives.begin()[e.PrimitiveIndex].Material;

			BindMaterial(renderSystem, commandBuffer, mat);
			
			{
				//materialSystem->BindSet(renderSystem, commandBuffer, Id("UIRenderGroup"), squareIndex);

				renderSystem->RenderMesh(uiRenderManager->GetSquareMesh());
			}

			++squareIndex;
		}
	}
}

void RenderOrchestrator::renderRays(GameInstance* gameInstance, RenderSystem* renderSystem, MaterialSystem* materialSystem, CommandBuffer commandBuffer, Id rp)
{
	auto* lightsRenderGroup = gameInstance->GetSystem<LightsRenderGroup>("LightsRenderGroup");
	
	for(auto& e : lightsRenderGroup->GetDirectionalLights()) //do a directional lights pass for every directional light
	{
		//todo: setup light data
		traceRays(renderSystem->GetRenderExtent(), &commandBuffer, renderSystem, materialSystem);
	}
}

void RenderOrchestrator::dispatch(GameInstance* gameInstance, RenderSystem* renderSystem, MaterialSystem* materialSystem, CommandBuffer commandBuffer, Id rp)
{	
	materialSystem->Dispatch(renderSystem->GetRenderExtent(), &commandBuffer, renderSystem);
}

void RenderOrchestrator::transitionImages(CommandBuffer commandBuffer, RenderSystem* renderSystem, MaterialSystem* materialSystem, Id renderPassId)
{
	GTSL::Array<CommandBuffer::BarrierData, 16> barriers;
	
	auto& renderPass = renderPassesMap.At(renderPassId);

	uint32 initialStage = 0;
	
	auto buildTextureBarrier = [&](const AttachmentData& attachmentData, PipelineStage::value_type attachmentStages, bool writeAccess)
	{
		auto& attachment = attachments.At(attachmentData.Name);

		CommandBuffer::TextureBarrier textureBarrier;
		textureBarrier.Texture = renderSystem->GetTexture(attachment.TextureHandle);
		textureBarrier.CurrentLayout = attachment.Layout;
		textureBarrier.TextureType = attachment.Type;
		textureBarrier.TargetLayout = attachmentData.Layout;
		textureBarrier.SourceAccessFlags = accessFlagsFromStageAndAccessType(attachment.ConsumingStages, attachment.WriteAccess);
		textureBarrier.DestinationAccessFlags = accessFlagsFromStageAndAccessType(attachmentStages, writeAccess);
		barriers.EmplaceBack(textureBarrier);

		initialStage |= attachment.ConsumingStages;
		
		updateImage(attachment, attachmentData.Layout, renderPass.PipelineStages, writeAccess);
	};
	
	for (auto& e : renderPass.ReadAttachments) { buildTextureBarrier(e, e.ConsumingStages, false); }
	for (auto& e : renderPass.WriteAttachments) { buildTextureBarrier(e, e.ConsumingStages, true); }
	
	commandBuffer.AddPipelineBarrier(renderSystem->GetRenderDevice(), barriers, initialStage, renderPass.PipelineStages, GetTransientAllocator());
}

void RenderOrchestrator::onShaderInfosLoaded(TaskInfo taskInfo, MaterialResourceManager* materialResourceManager,
	GTSL::Array<MaterialResourceManager::ShaderInfo, 8> shaderInfos, ShaderLoadInfo shaderLoadInfo)
{
	uint32 totalSize = 0;

	for (auto e : shaderInfos) { totalSize += e.Size; }

	shaderLoadInfo.Buffer.Allocate(totalSize, 8, GetPersistentAllocator());

	materialResourceManager->LoadShaders(taskInfo.GameInstance, shaderInfos, onShadersLoadHandle, shaderLoadInfo.Buffer.GetRange(), GTSL::MoveRef(shaderLoadInfo));
}

void RenderOrchestrator::onShadersLoaded(TaskInfo taskInfo, MaterialResourceManager*,
                                         GTSL::Array<MaterialResourceManager::ShaderInfo, 8> shaders, GTSL::Range<byte*> buffer,
                                         ShaderLoadInfo shaderLoadInfo)
{
	auto* renderSystem = taskInfo.GameInstance->GetSystem<RenderSystem>("RenderSystem");

	shaderLoadInfo.Component;

	ComputePipeline pipeline;
	ComputePipeline::CreateInfo createInfo;
	createInfo.RenderDevice = renderSystem->GetRenderDevice();
	createInfo.PipelineLayout;
	createInfo.ShaderInfo.Blob = GTSL::Range<const byte*>(shaders[0].Size, shaderLoadInfo.Buffer.GetData());
	createInfo.ShaderInfo.Type = ShaderType::COMPUTE;
	pipeline.Initialize(createInfo);
}

void RenderOrchestrator::traceRays(GTSL::Extent2D rayGrid, CommandBuffer* commandBuffer, RenderSystem* renderSystem, MaterialSystem* materialSystem)
{
	commandBuffer->BindPipeline(renderSystem->GetRenderDevice(), rayTracingPipeline, PipelineType::RAY_TRACING);

	auto handleSize = renderSystem->GetShaderGroupHandleSize();
	auto alignedHandleSize = GTSL::Math::RoundUpByPowerOf2(handleSize, renderSystem->GetShaderGroupBaseAlignment());

	auto alignedSize = GTSL::Math::RoundUpByPowerOf2(handleSize + 128, renderSystem->GetShaderGroupBaseAlignment());

	MaterialSystem::BufferIterator iterator;
	materialSystem->UpdateIteratorMember(iterator, sbtMemberHandle);

	auto bufferAddress = reinterpret_cast<uint64>(materialSystem->GetMemberPointer<uint32>(iterator));

	uint32 offset = 0;

	GTSL::Array<CommandBuffer::ShaderTableDescriptor, 4> shaderTableDescriptors;

	for (uint8 i = 0; i < 4; ++i)
	{
		CommandBuffer::ShaderTableDescriptor shaderTableDescriptor;

		shaderTableDescriptor.Entries = shaderCounts[i];
		shaderTableDescriptor.EntrySize = entrySizes[i];
		shaderTableDescriptor.Address = bufferAddress + offset;
		offset += shaderTableDescriptor.Entries * shaderTableDescriptor.EntrySize;

		shaderTableDescriptors.EmplaceBack(shaderTableDescriptor);
	}

	commandBuffer->TraceRays(renderSystem->GetRenderDevice(), shaderTableDescriptors, GTSL::Extent3D(rayGrid));
}

uint32 RenderOrchestrator::createTexture(const CreateTextureInfo& createTextureInfo)
{
	auto component = textureIndex++;

	pendingMaterialsPerTexture.EmplaceAt(component, GetPersistentAllocator());
	pendingMaterialsPerTexture[component].Initialize(4, GetPersistentAllocator());

	texturesRefTable.Emplace(createTextureInfo.TextureName, component);

	auto textureLoadInfo = TextureLoadInfo(component, createTextureInfo.RenderSystem, RenderAllocation());

	createTextureInfo.TextureResourceManager->LoadTextureInfo(createTextureInfo.GameInstance, createTextureInfo.TextureName, onTextureInfoLoadHandle, GTSL::MoveRef(textureLoadInfo));

	return component;
}

void RenderOrchestrator::onMaterialLoaded(TaskInfo taskInfo, MaterialResourceManager::OnMaterialLoadInfo onMaterialLoadInfo)
{
	auto loadInfo = DYNAMIC_CAST(MaterialLoadInfo, onMaterialLoadInfo.UserData);

	auto* materialSystem = taskInfo.GameInstance->GetSystem<MaterialSystem>("MaterialSystem");
	
	//taskInfo.GameInstance->DispatchEvent("GameInstance", GetOnMaterialLoadEventHandle(), Id(onMaterialLoadInfo.ResourceName));

	auto materialIndex = loadInfo->Component;
	auto& material = materials[materialIndex];

	auto* renderSystem = loadInfo->RenderSystem;

	material.MaterialInstances.Initialize(8, GetPersistentAllocator());

	material.RenderGroup = onMaterialLoadInfo.RenderGroup;
	material.Parameters = onMaterialLoadInfo.Parameters;
	material.Name = onMaterialLoadInfo.ResourceName;

	{
		RasterizationPipeline::CreateInfo createInfo;
		createInfo.RenderDevice = renderSystem->GetRenderDevice();
		if constexpr (_DEBUG) {
			GTSL::StaticString<64> name("Raster pipeline. Material: "); name += onMaterialLoadInfo.ResourceName();
			createInfo.Name = name;
		}

		GTSL::Array<ShaderDataType, 10> vertexDescriptor;
		for (auto e : onMaterialLoadInfo.VertexElements) { vertexDescriptor.EmplaceBack(ConvertShaderDataType(e)); }
		createInfo.VertexDescriptor = vertexDescriptor;

		createInfo.PipelineDescriptor.BlendEnable = onMaterialLoadInfo.BlendEnable; createInfo.PipelineDescriptor.CullMode = onMaterialLoadInfo.CullMode;
		createInfo.PipelineDescriptor.DepthTest = onMaterialLoadInfo.DepthTest; createInfo.PipelineDescriptor.DepthWrite = onMaterialLoadInfo.DepthWrite;
		createInfo.PipelineDescriptor.StencilTest = onMaterialLoadInfo.StencilTest; createInfo.PipelineDescriptor.DepthCompareOperation = GAL::CompareOperation::LESS;
		createInfo.PipelineDescriptor.ColorBlendOperation = onMaterialLoadInfo.ColorBlendOperation;

		auto transStencil = [](const MaterialResourceManager::StencilState& stencilState, GAL::StencilState& sS) {
			sS.CompareOperation = stencilState.CompareOperation; sS.CompareMask = stencilState.CompareMask;
			sS.DepthFailOperation = stencilState.DepthFailOperation; sS.FailOperation = stencilState.FailOperation;
			sS.PassOperation = stencilState.PassOperation; sS.Reference = stencilState.Reference; sS.WriteMask = stencilState.WriteMask;
		};

		transStencil(onMaterialLoadInfo.Front, createInfo.PipelineDescriptor.StencilOperations.Front);
		transStencil(onMaterialLoadInfo.Back, createInfo.PipelineDescriptor.StencilOperations.Back);

		createInfo.SurfaceExtent = { 1, 1 }; // Will be updated dynamically on render time

		GTSL::Array<Pipeline::ShaderInfo, 8> shaderInfos;

		{
			uint32 offset = 0;

			for (uint8 i = 0; i < onMaterialLoadInfo.ShaderSizes.GetLength(); ++i) {
				Pipeline::ShaderInfo shaderInfo;
				shaderInfo.Type = ConvertShaderType(onMaterialLoadInfo.ShaderTypes[i]);
				shaderInfo.Blob = GTSL::Range<const byte*>(onMaterialLoadInfo.ShaderSizes[i], loadInfo->Buffer.GetData() + offset);
				shaderInfos.EmplaceBack(shaderInfo);

				offset += onMaterialLoadInfo.ShaderSizes[i];
			}

			createInfo.Stages = shaderInfos;
		}

		{
			createInfo.RenderPass = getAPIRenderPass(onMaterialLoadInfo.RenderPass);
			createInfo.SubPass = getAPISubPassIndex(onMaterialLoadInfo.RenderPass);
			createInfo.AttachmentCount = GetRenderPassColorWriteAttachmentCount(onMaterialLoadInfo.RenderPass);
		}

		createInfo.PipelineLayout = materialSystem->GetSetLayoutPipelineLayout(Id("GlobalData"));
		createInfo.PipelineCache = renderSystem->GetPipelineCache();
		material.Pipeline = RasterizationPipeline(createInfo);
	}

	{
		GTSL::Array<MaterialSystem::MemberInfo, 16> materialParameters;

		for (auto& e : material.Parameters) {
			material.ParametersHandles.Emplace(e.Name);

			MaterialSystem::Member::DataType memberType;

			switch (e.Type)
			{
			case MaterialResourceManager::ParameterType::UINT32: memberType = MaterialSystem::Member::DataType::UINT32; break;
			case MaterialResourceManager::ParameterType::VEC4: memberType = MaterialSystem::Member::DataType::FVEC4; break;
			case MaterialResourceManager::ParameterType::TEXTURE_REFERENCE: memberType = MaterialSystem::Member::DataType::UINT32; break;
			case MaterialResourceManager::ParameterType::BUFFER_REFERENCE: memberType = MaterialSystem::Member::DataType::UINT64; break;
			}

			materialParameters.EmplaceBack(MaterialSystem::MemberInfo{ 1, memberType, &material.ParametersHandles.At(e.Name) });
		}

		GTSL::Array<MaterialSystem::MemberInfo, 1> materialInstanceParametersStruct;
		materialInstanceParametersStruct.EmplaceBack(MaterialSystem::MemberInfo{ 16, MaterialSystem::Member::DataType::STRUCT, &material.MaterialInstancesMemberHandle, materialParameters });

		auto bufferHandle = materialSystem->CreateBuffer(renderSystem, materialInstanceParametersStruct);
		materialSystem->BindBufferToName(bufferHandle, Id(onMaterialLoadInfo.ResourceName));
	}

	uint32 i = 0;

	for (auto& resourceMaterialInstance : onMaterialLoadInfo.MaterialInstances)
	{
		++material.InstanceCount;
		//UpdateObjectCount(renderSystem, material., material.InstanceCount); //assuming every material uses the same set instance, not index

		auto materialInstanceIndex = materialInstances.Emplace();
		materialInstancesByName.Emplace(resourceMaterialInstance.Name, materialInstanceIndex);

		auto instanceMaterialHandle = PrivateMaterialHandle{ loadInfo->Component, materialInstanceIndex, i };

		auto& materialInstance = materialInstances[materialInstanceIndex];
		materialInstance.Material = materialIndex;
		materialInstance.Name = resourceMaterialInstance.Name;

		//privateMaterialHandlesByName.Emplace(resourceMaterialInstance.Name(), instanceMaterialHandle);

		for (auto& resourceMaterialInstanceParameter : resourceMaterialInstance.Parameters)
		{
			auto materialParameter = material.Parameters.LookFor([&](const MaterialResourceManager::Parameter& parameter) { return parameter.Name == resourceMaterialInstanceParameter.First; }); //get parameter description from name

			BE_ASSERT(materialParameter.State(), "No parameter by that name found. Data must be invalid");

			if (material.Parameters[materialParameter.Get()].Type == MaterialResourceManager::ParameterType::TEXTURE_REFERENCE) //if parameter is texture reference, load texture
			{
				uint32 textureComponentIndex;

				auto textureReference = texturesRefTable.TryGet(resourceMaterialInstanceParameter.Second.TextureReference);

				if (!textureReference.State())
				{
					CreateTextureInfo createTextureInfo;
					createTextureInfo.RenderSystem = renderSystem;
					createTextureInfo.GameInstance = taskInfo.GameInstance;
					createTextureInfo.TextureResourceManager = loadInfo->TextureResourceManager;
					createTextureInfo.TextureName = resourceMaterialInstanceParameter.Second.TextureReference;
					createTextureInfo.MaterialHandle = instanceMaterialHandle;
					auto textureComponent = createTexture(createTextureInfo);

					addPendingMaterialToTexture(textureComponent, instanceMaterialHandle);

					textureComponentIndex = textureComponent;
				}
				else
				{
					textureComponentIndex = textureReference.Get();
					++materialInstance.Counter; //since we up the target for every texture, up the counter for every already existing texture
				}

				++materialInstance.Target;

				MaterialSystem::BufferIterator bufferIterator;
				//UpdateIteratorMember(bufferIterator, material.MaterialInstancesMemberHandle.Parameters.At(resourceMaterialInstanceParameter.First));
				materialSystem->UpdateIteratorMember(bufferIterator, material.MaterialInstancesMemberHandle);
				materialSystem->UpdateIteratorMemberIndex(bufferIterator, i);

				materialSystem->UpdateIteratorMember(bufferIterator, material.ParametersHandles.At(resourceMaterialInstanceParameter.First));

				materialSystem->WriteMultiBuffer(bufferIterator, &textureComponentIndex);
			}
		}

		++i;
	}

	loadedMaterials.Emplace(onMaterialLoadInfo.ResourceName, materialIndex);
	
	GTSL::Delete(loadInfo, GetPersistentAllocator());
}

void RenderOrchestrator::onTextureInfoLoad(TaskInfo taskInfo, TextureResourceManager* resourceManager,
	TextureResourceManager::TextureInfo textureInfo, TextureLoadInfo loadInfo)
{
	loadInfo.TextureHandle = loadInfo.RenderSystem->CreateTexture(textureInfo.Format, textureInfo.Extent, TextureUse::SAMPLE | TextureUse::COLOR_ATTACHMENT, true);

	auto dataBuffer = loadInfo.RenderSystem->GetTextureRange(loadInfo.TextureHandle);

	resourceManager->LoadTexture(taskInfo.GameInstance, textureInfo, dataBuffer, onTextureLoadHandle, GTSL::MoveRef(loadInfo));
}

void RenderOrchestrator::onTextureLoad(TaskInfo taskInfo, TextureResourceManager* resourceManager,
	TextureResourceManager::TextureInfo textureInfo, TextureLoadInfo loadInfo)
{
	auto* materialSystem = taskInfo.GameInstance->GetSystem<MaterialSystem>("MaterialSystem");
	
	loadInfo.RenderSystem->UpdateTexture(loadInfo.TextureHandle);

	materialSystem->WriteSetTexture(loadInfo.RenderSystem, textureSubsetsHandle, loadInfo.TextureHandle, loadInfo.Component);
	
	latestLoadedTextures.EmplaceBack(loadInfo.Component);
}

void RenderOrchestrator::setMaterialInstanceAsLoaded(const PrivateMaterialHandle privateMaterialHandle, const MaterialInstanceHandle materialInstanceHandle)
{
	//readyMaterials.EmplaceBack(privateMaterialHandle.MaterialInstance);

	auto& material = materials[privateMaterialHandle.MaterialIndex];
	material.MaterialInstances.EmplaceBack(privateMaterialHandle.MaterialInstance, privateMaterialHandle.SubMaterialIndex);
	
	{
		auto result = awaitingMaterialInstances.TryGet(materialInstanceHandle);
	
		if (result.State()) {
			materialInstances[privateMaterialHandle.MaterialInstance].Meshes = GTSL::MoveRef(result.Get().Meshes);
			loadedMaterialInstances.Emplace(materialInstanceHandle, privateMaterialHandle.MaterialInstance);
		}
	}
}
