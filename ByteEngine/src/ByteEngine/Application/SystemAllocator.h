#pragma once

#include "ByteEngine/Core.h"
#include <GTSL/Memory.h>
#include <GTSL/Mutex.h>
#include <GTSL/Math/Math.hpp>

/**
 * \brief Allocates memory directly from the OS. Useful for all other allocators.
 */
class SystemAllocator
{
public:
	struct DebugData
	{
		uint64 AllocatedBytes{ 0 };
		uint64 DeallocatedBytes{ 0 };
		uint64 TotalAllocatedBytes{ 0 };
		uint64 TotalDeallocatedBytes{ 0 };
		uint64 AllocationCount{ 0 };
		uint64 TotalAllocationCount{ 0 };
	};
protected:
	GTSL::Mutex allocatorMutex;
	
#if BE_DEBUG
	GTSL::Mutex debugDataMutex;
	uint64 allocatedBytes{ 0 };
	uint64 deallocatedBytes{ 0 };
	uint64 totalAllocatedBytes{ 0 };
	uint64 totalDeallocatedBytes{ 0 };
	uint64 allocationCount{ 0 };
	uint64 totalAllocationCount{ 0 };
#endif
	
public:
	SystemAllocator()
	{
		
	}

#if BE_DEBUG
	void GetDebugData(DebugData& debugData)
	{
		GTSL::Lock<GTSL::Mutex> lock(debugDataMutex);
		debugData.AllocationCount = allocationCount;
		debugData.AllocatedBytes = allocatedBytes;
		debugData.DeallocatedBytes = deallocatedBytes;
		debugData.TotalAllocatedBytes = totalAllocatedBytes;
		debugData.TotalDeallocatedBytes = totalDeallocatedBytes;
		debugData.TotalAllocationCount = totalAllocationCount;

		allocationCount = 0;
		allocatedBytes = 0;
		deallocatedBytes = 0;
	}
#endif
	
	void Allocate(const uint64 size, const uint64 alignment, void** data)
	{
		const uint64 allocated_size{ GTSL::Math::PowerOf2RoundUp(size, alignment) };
		void* raw_data_alloc{ nullptr };
		
		allocatorMutex.Lock();
		GTSL::Allocate(size, &raw_data_alloc);
		allocatorMutex.Unlock();
		
		//*data = GTSL::Memory::AlignedPointer(alignment, raw_data_alloc);
		//::new(static_cast<byte*>(*data)) (void*)(raw_data_alloc);

		*data = raw_data_alloc;
		
		BE_DEBUG_ONLY(GTSL::Lock<GTSL::Mutex> lock(debugDataMutex))
		BE_DEBUG_ONLY(allocatedBytes += size)
		BE_DEBUG_ONLY(totalAllocatedBytes += size)
		BE_DEBUG_ONLY(++allocationCount)
		BE_DEBUG_ONLY(++totalAllocationCount)
	}

	void Deallocate(const uint64 size, const uint64 alignment, void* data)
	{
		const uint64 deallocated_size{ GTSL::Math::PowerOf2RoundUp(size, alignment) };

		allocatorMutex.Lock();
		GTSL::Deallocate(size, data);
		allocatorMutex.Unlock();
		
		BE_DEBUG_ONLY(GTSL::Lock<GTSL::Mutex> lock(debugDataMutex))
		BE_DEBUG_ONLY(deallocatedBytes += size)
		BE_DEBUG_ONLY(totalDeallocatedBytes += size)
		BE_DEBUG_ONLY(++allocationCount)
		BE_DEBUG_ONLY(++totalAllocationCount)
	}
};
