#pragma once


#include "ByteEngine/Object.h"
#include <GTSL/Id.h>

using ComponentReference = uint32;

namespace GTSL
{
	template<typename T, class ALLOC>
	class Vector;
}

template<typename T, class ALLOC = BE::PersistentAllocatorReference>
using Vector = GTSL::Vector<T, ALLOC>;

using Id = GTSL::Id64;
using Id32 = GTSL::Id32;

/**
 * \brief Systems persist across levels and can process world components regardless of the current level.
 * Used to instantiate render engines, sound engines, physics engines, AI systems, etc.
 */
class System : public Object
{
public:
	System() = default;
	
	System(const UTF8* name) : Object(name)
	{
	}
	
	using ComponentReference = uint32;

	struct InitializeInfo
	{
		class GameInstance* GameInstance{ nullptr };
	};
	virtual void Initialize(const InitializeInfo& initializeInfo) = 0;

	struct ShutdownInfo
	{
		class GameInstance* GameInstance = nullptr;
	};
	virtual void Shutdown(const ShutdownInfo& shutdownInfo) = 0;
private:
};
