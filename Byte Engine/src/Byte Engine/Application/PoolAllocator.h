#pragma once

#include "Byte Engine/Core.h"

#include <atomic>
#include <GTSL/Allocator.h>
#include <GTSL/Mutex.h>
#include <GTSL/Ranger.h>

class PoolAllocator
{
	GTSL::AllocatorReference* systemAllocatorReference{ nullptr };

	class Pool
	{
		struct Block
		{
		protected:
			void* data{ nullptr };
			std::atomic<uint16> freeSlotsCount{ 0 };
			//uint16 freeSlotsCount{ 0 };
			//GTSL::Mutex mutex;

			[[nodiscard]] uint32* freeSlotsIndices() const { return reinterpret_cast<uint32*>(static_cast<byte*>(data)); }
			[[nodiscard]] byte* blockData(const uint16 slotsCount) const { return reinterpret_cast<byte*>(freeSlotsIndices()) + sizeof(uint32) * slotsCount; }
			[[nodiscard]] byte* blockDataEnd(const uint16 slotsCount, const uint32 slotsSize) const { return blockData(slotsCount) + static_cast<uint64>(slotsSize) * slotsCount; }
			
			void popFreeSlot(uint32& freeSlot)
			{
				//freeSlot = freeSlotsIndices()[freeSlotsCount.fetch_add(1)];
				freeSlot = freeSlotsIndices()[freeSlotsCount++];
			}

			void placeFreeSlot(const uint32 freeSlot)
			{
				//freeSlotsIndices()[freeSlotsCount.fetch_sub(1)] = freeSlot;
				freeSlotsIndices()[freeSlotsCount--] = freeSlot;
			}
			
			[[nodiscard]] bool freeSlot() const { return freeSlotsCount;/*freeSlotsCount.load(std::memory_order::memory_order_seq_cst);*/ }

			uint32 slotIndexFromPointer(void* p, const uint16 slotsCount, const uint32 slotsSize) const
			{
				BE_ASSERT(p > blockDataEnd(slotsCount, slotsSize) || p < blockData(slotsCount), "p does not belong to block!");
				return static_cast<uint32>((blockDataEnd(slotsCount, slotsSize) - static_cast<byte*>(p))) / slotsSize;
			}
			
		public:
			Block(uint16 slotsCount, uint32 slotsSize, uint64& allocatedSize, GTSL::AllocatorReference* allocatorReference);

			void FreeBlock(uint16 slotsCount, uint32 slotsSize, uint64& freedSpace, GTSL::AllocatorReference* allocatorReference);

			bool DoesAllocationBelongToBlock(void* p, const uint16 slotsCount, const uint32 slotsSize) const { return p > blockData(slotsCount) && p < blockDataEnd(slotsCount, slotsSize); }

			void AllocateInBlock(uint64 alignment, void** data, uint64& allocatedSize, uint16 slotsCount, uint32 slotsSize);

			bool TryAllocateInBlock(uint64 alignment, void** data, uint64& allocatedSize, uint16 slotsCount, uint32 slotsSize);

			void DeallocateInBlock(uint64 alignment, void* data, const uint16 slotsCount, const uint32 slotsSize)
			{
				placeFreeSlot(slotIndexFromPointer(data, slotsCount, slotsSize));
			}
		};

		//std::atomic<Block*> blocks{ nullptr };
		std::atomic<uint8> blockCount{ 0 };
		std::atomic<uint8> blockCapacity{ 0 };
		std::atomic<uint8> index{ 0 };
		
		Block* blocks{ nullptr };
		//uint32 blockCount{ 0 };
		//uint32 blockCapacity{ 0 };
		//uint32 index{ 0 };
		GTSL::ReadWriteMutex mutex;
		const uint32 slotsSize{ 0 };
		const uint16 slotsCount{ 0 };

		uint32 allocateAndAddNewBlock(GTSL::AllocatorReference* allocatorReference);
		[[nodiscard]] auto blocksRange() const { return Ranger(blocks, blocks + blockCount); }//Ranger(blocks.load(), blocks + blockCount); }
	public:
		Pool(uint16 slotsCount, uint32 slotsSize, uint8 blockCount, uint64& allocatedSize, GTSL::AllocatorReference* allocatorReference);

		void Allocate(uint64 size, uint64 alignment, void** data, uint64* allocatedSize, uint64& allocatorAllocatedBytes, GTSL::AllocatorReference* allocatorReference);

		void Deallocate(uint64 size, uint64 alignment, void* memory, GTSL::AllocatorReference* allocatorReference) const;

		void Free(uint64& freedBytes, GTSL::AllocatorReference* allocatorReference) const;
	};
	
	Pool* pools{ nullptr };
	const uint32 poolCount{ 0 };
public:
	PoolAllocator(GTSL::AllocatorReference* allocatorReference);

	~PoolAllocator()
	{
		Free();
	}

	void Allocate(uint64 size, uint64 alignment, void** memory, uint64* allocatedSize, const char* name);

	void Deallocate(uint64 size, uint64 alignment, void* memory, const char* name);
	
	void Free();
};
