#include "AnimationResourceManager.h"

#include <assimp/postprocess.h>
#include <GTSL/Buffer.hpp>

void AnimationResourceManager::loadSkeleton(const GTSL::Buffer<BE::TAR>& sourceBuffer, SkeletonData& skeletonData,
	GTSL::Buffer<BE::TAR>& meshDataBuffer)
{
	Assimp::Importer importer;
	const auto* const scene = importer.ReadFileFromMemory(sourceBuffer.GetData(), sourceBuffer.GetLength(),
		aiProcess_Triangulate | aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals |
		aiProcess_JoinIdenticalVertices);

	if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) { return; }

	aiMesh* mesh = scene->mMeshes[0];

	skeletonData.Bones.Initialize(mesh->mNumBones, GetPersistentAllocator());
	skeletonData.BonesMap.Initialize(mesh->mNumBones, GetPersistentAllocator());
	
	for (uint32 b = 0; b < mesh->mNumBones; ++b)
	{
		const auto& assimpBone = mesh->mBones[b];
		
		skeletonData.BonesMap.Emplace(assimpStringToId(assimpBone->mName)(), skeletonData.Bones.GetLength());
		auto& bone = skeletonData.Bones.EmplaceBack();

		for (uint32 w = 0; w < assimpBone->mNumWeights; ++w)
		{
			bone.AffectedBone[w] = assimpBone->mWeights[w].mVertexId;
			bone.EffectIntensity[w] = assimpBone->mWeights[w].mWeight;
		}

		bone.Offset = assimpMatrixToMatrix(mesh->mBones[b]->mOffsetMatrix);
	}
}

void AnimationResourceManager::loadAnimation(const GTSL::Buffer<BE::TAR>& sourceBuffer, AnimationData& animationData,
	GTSL::Buffer<BE::TAR>& meshDataBuffer)
{
	Assimp::Importer importer;
	const auto* const scene = importer.ReadFileFromMemory(sourceBuffer.GetData(), sourceBuffer.GetLength(),
		aiProcess_Triangulate | aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals |
		aiProcess_JoinIdenticalVertices);

	if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) { return; }

	aiMesh* mesh = scene->mMeshes[0];

	animationData.Frames.Initialize(static_cast<uint32>(scene->mAnimations[0]->mDuration), GetPersistentAllocator());

	GTSL::Vector<Id, BE::TAR> boneNames(128, GetTransientAllocator());

	for (uint32 b = 0; b < mesh->mNumBones; ++b)
	{
		const auto& assimpBone = mesh->mBones[b];
		auto name = assimpStringToId(assimpBone->mName);
		boneNames.EmplaceBack(name);
	}
	
	for (uint32 a = 0; a < scene->mNumAnimations; ++a)
	{
		auto& assimpAnimation = scene->mAnimations[a]; //a single animation, e.g. "walk", "run", "shoot"
		auto animationName = assimpStringToId(assimpAnimation->mName);

		AnimationData animation;

		BE_ASSERT(assimpAnimation->mDuration - ((float64)(uint64)assimpAnimation->mDuration) != 0.0,
			"Animation is not round number");
		animation.FrameCount = static_cast<uint64>(assimpAnimation->mDuration);

		animation.FPS = static_cast<float32>(assimpAnimation->mTicksPerSecond == 0.0
			? 30.0
			: assimpAnimation->mTicksPerSecond);

		for (uint32 c = 0; c < assimpAnimation->mNumChannels; ++c) {
			BE_ASSERT(assimpStringToId(assimpAnimation->mChannels[c]->mNodeName) == boneNames[c], "Order doesn't match");
		}

		for (uint32 frameIndex = 0; frameIndex < animation.FrameCount; ++frameIndex)
		{
			auto& frame = animation.Frames.EmplaceBack();
			frame.Bones.Initialize(assimpAnimation->mNumChannels, GetPersistentAllocator());
			
			for (uint32 b = 0; b < assimpAnimation->mNumChannels; ++b)
			{
				const auto& assimpChannel = assimpAnimation->mChannels[b];

				auto nodeName = assimpStringToId(assimpChannel->mNodeName);

				if (assimpChannel->mNumPositionKeys != assimpChannel->mNumRotationKeys != assimpChannel->mNumScalingKeys)
				{
					BE_LOG_WARNING("Number of keys doesn't match");
				}

				auto& bone = frame.Bones.EmplaceBack();

				BE_ASSERT(
					assimpChannel->mPositionKeys[frameIndex].mTime ==
					assimpChannel->mRotationKeys[frameIndex].mTime ==
					assimpChannel->mScalingKeys[frameIndex].mTime, "Time doesn't match");

				bone.Position = aiVector3DToVector(assimpChannel->mPositionKeys[frameIndex].mValue);
				bone.Rotation = aiQuaternionToQuaternion(assimpChannel->mRotationKeys[frameIndex].mValue);
				bone.Scale = aiVector3DToVector(assimpChannel->mScalingKeys[frameIndex].mValue);
			}
		}
	}
}
