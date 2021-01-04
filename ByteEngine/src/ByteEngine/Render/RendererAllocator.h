#pragma once
#include <GTSL/DataSizes.h>
#include <GTSL/Vector.hpp>
#include <GTSL/Math/Math.hpp>


#include "RenderTypes.h"
#include "ByteEngine/Core.h"
#include "ByteEngine/Application/AllocatorReferences.h"

class ScratchMemoryAllocator;

struct FreeSpace
{
	FreeSpace(const uint32 size, const uint32 offset) : Size(size), Offset(offset)
	{
	}

	uint32 Size = 0;
	uint32 Offset = 0;
};

struct AllocID
{
	uint32 Index = 0;
	uint32 BlockInfo = 0;

	AllocID() = default;

	AllocID(const AllocationId& allocation) : Index(static_cast<uint32>(allocation)), BlockInfo(allocation >> 32) {}

	operator AllocationId() const { return static_cast<uint64>(BlockInfo) << 32 | Index; }
};

struct LocalMemoryBlock
{
	void Initialize(const RenderDevice& renderDevice, uint32 size, uint32 memType, const BE::PersistentAllocatorReference& allocatorReference);
	void Free(const RenderDevice& renderDevice, const BE::PersistentAllocatorReference& allocatorReference);

	bool TryAllocate(DeviceMemory* deviceMemory, uint32 size, uint32* offset);
	void Allocate(DeviceMemory* deviceMemory, uint32 size, uint32* offset, uint32& id);
	void Deallocate(uint32 size, uint32 offset, uint32 id);

private:
	DeviceMemory deviceMemory;

	GTSL::Vector<FreeSpace, BE::PersistentAllocatorReference> freeSpaces;
};

class LocalMemoryAllocator : public Object
{
public:
	LocalMemoryAllocator() : Object("LocalMemoryAllocator") {}

	void Initialize(const RenderDevice& renderDevice, const BE::PersistentAllocatorReference& allocatorReference);
	
	void Free(const RenderDevice& renderDevice, const BE::PersistentAllocatorReference& allocatorReference);

	void AllocateBuffer(const RenderDevice& renderDevice, DeviceMemory* deviceMemory, RenderAllocation* renderAllocation, const BE::PersistentAllocatorReference& allocatorReference);
	
	void DeallocateBuffer(const RenderDevice& renderDevice, const RenderAllocation allocation)
	{
		if constexpr (!SINGLE_ALLOC)
		{
			const auto alloc = AllocID(allocation.AllocationId);
			bufferMemoryBlocks[alloc.Index].Deallocate(GTSL::Math::RoundUpByPowerOf2(allocation.Size, bufferMemoryAlignment), allocation.Offset, alloc.BlockInfo);
		}
	}

	void AllocateTexture(const RenderDevice& renderDevice, DeviceMemory* deviceMemory, RenderAllocation* renderAllocation, const BE::PersistentAllocatorReference& persistentAllocatorReference);
	void DeallocateTexture(const RenderDevice& renderDevice, const RenderAllocation allocation)
	{
		if constexpr (!SINGLE_ALLOC)
		{
			const auto alloc = AllocID(allocation.AllocationId);
			textureMemoryBlocks[alloc.Index].Deallocate(GTSL::Math::RoundUpByPowerOf2(allocation.Size, textureMemoryAlignment), allocation.Offset, alloc.BlockInfo);
		}
	}

private:
	static constexpr GTSL::Byte ALLOCATION_SIZE{ GTSL::MegaByte(128) };

	static constexpr bool SINGLE_ALLOC = false;
	
	uint32 bufferMemoryType = 0, textureMemoryType = 0;
	
	GTSL::Array<LocalMemoryBlock, 32> bufferMemoryBlocks;
	GTSL::Array<LocalMemoryBlock, 32> textureMemoryBlocks;
	uint32 bufferMemoryAlignment = 0, textureMemoryAlignment = 0;
};


struct ScratchMemoryBlock
{
	ScratchMemoryBlock() = default;

	void Initialize(const RenderDevice& renderDevice, uint32 size, uint32 memType, const BE::PersistentAllocatorReference& allocatorReference);
	void Free(const RenderDevice& renderDevice, const BE::PersistentAllocatorReference& allocatorReference);

	bool TryAllocate(DeviceMemory* deviceMemory, uint32 size, uint32* offset, void** data, uint32& id);
	void AllocateFirstBlock(DeviceMemory* deviceMemory, uint32 size, uint32* offset, void** data, uint32& id);
	void Deallocate(uint32 size, uint32 offset, uint32 id);
private:
	DeviceMemory deviceMemory;
	void* mappedMemory = nullptr;

	GTSL::Vector<FreeSpace, BE::PersistentAllocatorReference> freeSpaces;
};

class ScratchMemoryAllocator : public Object
{
public:
	ScratchMemoryAllocator() : Object("ScratchMemoryAllocator") {}

	void Initialize(const RenderDevice& renderDevice, const BE::PersistentAllocatorReference& allocatorReference);
	
	void AllocateBuffer(const RenderDevice& renderDevice, DeviceMemory* deviceMemory, HostRenderAllocation* renderAllocation, const BE::PersistentAllocatorReference& allocatorReference);
	void DeallocateBuffer(const RenderDevice& renderDevice, const HostRenderAllocation allocation)
	{
		if constexpr (!SINGLE_ALLOC)
		{
			const auto alloc = AllocID(allocation.AllocationId);
			bufferMemoryBlocks[alloc.Index].Deallocate(GTSL::Math::RoundUpByPowerOf2(allocation.Size, bufferMemoryAlignment), allocation.Offset, alloc.BlockInfo);
		}
	}
	
	void Free(const RenderDevice& renderDevice, const BE::PersistentAllocatorReference& allocatorReference);

private:
	static constexpr GTSL::Byte ALLOCATION_SIZE{ GTSL::MegaByte(128) };

	static constexpr bool SINGLE_ALLOC = false;
	
	uint32 bufferMemoryType = 0;

	uint32 bufferMemoryAlignment = 0;
	
	GTSL::Array<ScratchMemoryBlock, 32> bufferMemoryBlocks;
};