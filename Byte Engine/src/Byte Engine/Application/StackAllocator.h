#pragma once
#include <GTSL/Math/Math.hpp>
#include <GTSL/FixedVector.hpp>

class StackAllocator
{
	struct Block
	{
		byte* start{ nullptr };
		byte* at{ nullptr };
		byte* end{ nullptr };

		void AllocateBlock(const uint64 minimumSize, GTSL::AllocatorReference* allocatorReference, uint64& allocatedSize)
		{
			uint64 allocated_size{0};
			
			allocatorReference->Allocate(minimumSize, alignof(byte), reinterpret_cast<void**>(&start), &allocated_size);
			
			allocatedSize = allocated_size;
			
			at = start;
			end = start + allocated_size;
		}

		void DeallocateBlock(GTSL::AllocatorReference* allocatorReference, uint64& deallocatedBytes) const
		{
			allocatorReference->Deallocate(end - start, alignof(byte), start);
			deallocatedBytes = end - start;
		}

		void AllocateInBlock(const uint64 size, const uint64 alignment, void** data, uint64& allocatedSize)
		{
			*data = (at += (allocatedSize = GTSL::Math::AlignedNumber(size, alignment)));
		}
		
		bool TryAllocateInBlock(const uint64 size, const uint64 alignment, void** data, uint64& allocatedSize)
		{
			const auto new_at = at + (allocatedSize = GTSL::Math::AlignedNumber(size, alignment));
			if (new_at < end) { *data = new_at; at = new_at; return true; }
			return false;
		}

		void Clear() { at = start; }

		[[nodiscard]] bool FitsInBlock(const uint64 size, uint64 alignment) const { return at + size < end; }

		[[nodiscard]] uint64 GetBlockSize() const { return end - start; }
		[[nodiscard]] uint64 GetRemainingSize() const { return end - at; }
	};

public:
	struct DebugData
	{
		DebugData(GTSL::AllocatorReference* allocatorReference)
		{
		}
		
		struct PerNameData
		{
			const char* Name{ nullptr };
			uint64 AllocationCount{ 0 };
			uint64 DeallocationCount{ 0 };
			uint64 BytesAllocated{ 0 };
			uint64 BytesDeallocated{ 0 };
		};
		
		std::unordered_map<GTSL::Id64::HashType, PerNameData> PerNameAllocationsData;
		
		/**
		 * \brief Number of times it was tried to allocate on different blocks to no avail.
		 * To improve this number(lower it) try to make the blocks bigger.
		 * Don't make it so big that in the event that a new block has to be allocated it takes up too much space.
		 * Reset to 0 on every call to GetDebugInfo()
		 */
		uint64 BlockMisses{ 0 };

		uint64 BytesAllocated{ 0 };
		uint64 TotalBytesAllocated{ 0 };
		
		uint64 BytesDeallocated{ 0 };
		uint64 TotalBytesDeallocated{ 0 };
		
		uint64 AllocatorAllocatedBytes{ 0 };
		uint64 TotalAllocatorAllocatedBytes{ 0 };

		uint64 AllocatorDeallocatedBytes{ 0 };
		uint64 TotalAllocatorDeallocatedBytes{ 0 };
		
		uint64 AllocationsCount{ 0 };
		uint64 TotalAllocationsCount{ 0 };
		
		uint64 DeallocationsCount{ 0 };
		uint64 TotalDeallocationsCount{ 0 };
		
		uint64 AllocatorAllocationsCount{ 0 };
		uint64 TotalAllocatorAllocationsCount{ 0 };
		
		uint64 AllocatorDeallocationsCount{ 0 };
		uint64 TotalAllocatorDeallocationsCount{ 0 };
	};
protected:
	const uint64 blockSize{ 0 };
	std::atomic<uint32> stackIndex{ 0 };
	GTSL::Vector<GTSL::Vector<Block>> stacks;
	GTSL::Vector<GTSL::Mutex> stacksMutexes;
	GTSL::AllocatorReference* allocatorReference{ nullptr };

#if BE_DEBUG
	uint64 blockMisses{ 0 };
	std::unordered_map<GTSL::Id64::HashType, DebugData::PerNameData> perNameData;
	GTSL::Mutex debugDataMutex;
	
	uint64 bytesAllocated{ 0 };
	uint64 bytesDeallocated{ 0 };
	
	uint64 totalAllocatorAllocatedBytes{ 0 };
	uint64 totalAllocatorDeallocatedBytes{ 0 };
	
	uint64 allocationsCount{ 0 };
	uint64 deallocationsCount{ 0 };
	
	uint64 allocatorAllocationsCount{ 0 };
	uint64 allocatorDeallocationsCount{ 0 };

	uint64 allocatorAllocatedBytes{ 0 };
	uint64 allocatorDeallocatedBytes{ 0 };
	
	uint64 totalBytesAllocated{ 0 };
	uint64 totalBytesDeallocated{ 0 };
	
	uint64 totalAllocationsCount{ 0 };
	uint64 totalDeallocationsCount{ 0 };
	
	uint64 totalAllocatorAllocationsCount{ 0 };
	uint64 totalAllocatorDeallocationsCount{ 0 };
#endif
	
	const uint8 maxStacks{ 8 };
	
public:
	explicit StackAllocator(GTSL::AllocatorReference* allocatorReference, const uint8 stackCount = 8, const uint8 defaultBlocksPerStackCount = 2, const uint64 blockSizes = 512) : blockSize(blockSizes), stacks(stackCount, allocatorReference), stacksMutexes(stackCount, allocatorReference), allocatorReference(allocatorReference), maxStacks(stackCount)
	{
		uint64 allocated_size = 0;
		
		for(uint8 i = 0; i < stackCount; ++i)
		{
			stacks.EmplaceBack(defaultBlocksPerStackCount, allocatorReference); //construct stack [i]s
			
			for (uint32 j = 0; j < defaultBlocksPerStackCount; ++j) //for every block in constructed vector
			{
				stacks[i].EmplaceBack(); //construct a default block
				
				stacks[i][j].AllocateBlock(blockSizes, allocatorReference, allocated_size); //allocate constructed block, which is also current block

				BE_DEBUG_ONLY(GTSL::Lock<GTSL::Mutex> lock(debugDataMutex))
				
				BE_DEBUG_ONLY(++allocatorAllocationsCount)
				BE_DEBUG_ONLY(++totalAllocatorAllocationsCount)
				
				BE_DEBUG_ONLY(allocatorAllocatedBytes += allocated_size)
				BE_DEBUG_ONLY(totalAllocatorAllocatedBytes += allocated_size)
			}
			
			stacksMutexes.EmplaceBack();
		}
	}

	~StackAllocator()
	{
		uint64 deallocatedBytes{ 0 };
		
		for (auto& stack : stacks)
		{
			for (auto& block : stack)
			{
				block.DeallocateBlock(allocatorReference, deallocatedBytes);
				
				BE_DEBUG_ONLY(++allocatorDeallocationsCount)
				BE_DEBUG_ONLY(++totalAllocatorDeallocationsCount)
				
				BE_DEBUG_ONLY(allocatorDeallocatedBytes += deallocatedBytes)
				BE_DEBUG_ONLY(totalAllocatorDeallocatedBytes += deallocatedBytes)
			}
		}
	}

#if BE_DEBUG
	void GetDebugData(DebugData& debugData)
	{
		GTSL::Lock<GTSL::Mutex> lock(debugDataMutex);
		
		debugData.BlockMisses = blockMisses;
		
		debugData.PerNameAllocationsData = perNameData;
		
		debugData.AllocationsCount = allocationsCount;
		debugData.TotalAllocationsCount = totalAllocationsCount;

		debugData.DeallocationsCount = deallocationsCount;
		debugData.TotalDeallocationsCount = totalDeallocationsCount;
		
		debugData.BytesAllocated = bytesAllocated;
		debugData.TotalBytesAllocated = totalBytesAllocated;
		
		debugData.BytesDeallocated = bytesDeallocated;
		debugData.TotalBytesDeallocated = totalBytesDeallocated;
		
		debugData.AllocatorAllocationsCount = allocatorAllocationsCount;
		debugData.TotalAllocatorAllocationsCount = totalAllocatorAllocationsCount;
		
		debugData.AllocatorDeallocationsCount = allocatorDeallocationsCount;
		debugData.TotalAllocatorDeallocationsCount = totalAllocatorDeallocationsCount;

		debugData.AllocatorAllocatedBytes = allocatorAllocatedBytes;
		debugData.TotalAllocatorAllocatedBytes = totalAllocatorAllocatedBytes;

		debugData.AllocatorDeallocatedBytes = allocatorDeallocatedBytes;
		debugData.TotalAllocatorDeallocatedBytes = totalAllocatorDeallocatedBytes;
		
		for (auto& e : perNameData)
		{
			e.second.DeallocationCount = 0;
			e.second.AllocationCount = 0;
			e.second.BytesAllocated = 0;
			e.second.BytesDeallocated = 0;
		}
		
		blockMisses = 0;
		
		bytesAllocated = 0;
		bytesDeallocated = 0;
		
		allocationsCount = 0;
		deallocationsCount = 0;
		
		allocatorAllocationsCount = 0;
		allocatorDeallocationsCount = 0;

		allocatorAllocatedBytes = 0;
		allocatorDeallocatedBytes = 0;
	}
#endif
	
	void Clear()
	{
		for(auto& stack : stacks)
		{
			for (auto& block : stack)
			{
				block.Clear();
			}
		}
		
		stackIndex = 0;
	}

	void LockedClear()
	{
		uint64 n{ 0 };
		for(auto& stack : stacks)
		{
			for (auto& block : stack)
			{
				stacksMutexes[n].Lock();
				block.Clear();
				stacksMutexes[n].Unlock();
			}
			++n;
		}

		stackIndex = 0;
	}

	void Allocate(const uint64 size, const uint64 alignment, void** memory, uint64* allocatedSize, const char* name)
	{
		uint64 n{ 0 };
		const auto i{ stackIndex % maxStacks };

		++stackIndex;
		
		BE_ASSERT((alignment & (alignment - 1)) != 0, "Alignment is not power of two!")
		BE_ASSERT(size > blockSize, "Single allocation is larger than block sizes! An allocation larger than block size can't happen.")
		
		uint64 allocated_size{ 0 };

		{
			BE_DEBUG_ONLY(GTSL::Lock<GTSL::Mutex> lock(debugDataMutex));
			BE_DEBUG_ONLY(perNameData.try_emplace(GTSL::Id64(name)).first->second.Name = name)
		}
			
		stacksMutexes[i].Lock();
		for(uint32 j = 0; j < stacks[i].GetLength(); ++j)
		{
			if (stacks[j][n].TryAllocateInBlock(size, alignment, memory, allocated_size))
			{
				stacksMutexes[i].Unlock();
				*allocatedSize = allocated_size;
				
				BE_DEBUG_ONLY(GTSL::Lock<GTSL::Mutex> lock(debugDataMutex));
				
				BE_DEBUG_ONLY(perNameData[GTSL::Id64(name)].BytesAllocated += allocated_size)
				BE_DEBUG_ONLY(perNameData[GTSL::Id64(name)].AllocationCount += 1)
				
				BE_DEBUG_ONLY(bytesAllocated += allocated_size)
				BE_DEBUG_ONLY(totalBytesAllocated += allocated_size)
				
				BE_DEBUG_ONLY(++allocationsCount)
				BE_DEBUG_ONLY(++totalAllocationsCount)
				
				return;
			}

			++n;
			BE_DEBUG_ONLY(debugDataMutex.Lock())
			BE_DEBUG_ONLY(++blockMisses)
			BE_DEBUG_ONLY(debugDataMutex.Unlock())
		}

		//stacks[i].EmplaceBack();
		stacks[i][n].AllocateBlock(blockSize, allocatorReference, allocated_size);
		stacks[i][n].AllocateInBlock(size, alignment, memory, allocated_size);
		stacksMutexes[i].Unlock();
		*allocatedSize = allocated_size;

		BE_DEBUG_ONLY(GTSL::Lock<GTSL::Mutex> lock(debugDataMutex));
		
		BE_DEBUG_ONLY(perNameData[GTSL::Id64(name)].BytesAllocated += allocated_size)
		BE_DEBUG_ONLY(perNameData[GTSL::Id64(name)].AllocationCount += 1)
		
		BE_DEBUG_ONLY(bytesAllocated += allocated_size)
		BE_DEBUG_ONLY(totalBytesAllocated += allocated_size)

		BE_DEBUG_ONLY(allocatorAllocatedBytes += allocated_size)
		BE_DEBUG_ONLY(totalAllocatorAllocatedBytes += allocated_size)
		
		BE_DEBUG_ONLY(++allocatorAllocationsCount)
		BE_DEBUG_ONLY(++totalAllocatorAllocationsCount)
		
		BE_DEBUG_ONLY(++allocationsCount)
		BE_DEBUG_ONLY(++totalAllocationsCount)
	}

	void Deallocate(const uint64 size, const uint64 alignment, void* memory, const char* name)
	{
		BE_ASSERT((alignment & (alignment - 1)) != 0, "Alignment is not power of two!")
		BE_ASSERT(size > blockSize, "Deallocation size is larger than block size! An allocation larger than block size can't happen. Trying to deallocate more bytes than allocated!")
		
		BE_DEBUG_ONLY(const auto bytes_deallocated{ GTSL::Math::AlignedNumber(size, alignment) })
		
		BE_DEBUG_ONLY(GTSL::Lock<GTSL::Mutex> lock(debugDataMutex));
		
		BE_DEBUG_ONLY(perNameData[GTSL::Id64(name)].BytesDeallocated += bytes_deallocated)
		BE_DEBUG_ONLY(perNameData[GTSL::Id64(name)].DeallocationCount += 1)
		
		BE_DEBUG_ONLY(bytesDeallocated += bytes_deallocated)
		BE_DEBUG_ONLY(totalBytesDeallocated += bytes_deallocated)
		
		BE_DEBUG_ONLY(++deallocationsCount)
		BE_DEBUG_ONLY(++totalDeallocationsCount)
	}
};
