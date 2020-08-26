#pragma once

#include "ByteEngine/Core.h"

#include <GTSL/Id.h>
#include <GTSL/Vector.hpp>
#include <GTSL/Flags.h>
#include <GTSL/Result.h>

#include "ByteEngine/Id.h"
#include "ByteEngine/Debug/Assert.h"

//enum class AccessType : uint8 { READ = 1, READ_WRITE = 4 };

struct AccessType : GTSL::Flags<uint8>
{	
	static constexpr value_type READ = 1;
	static constexpr value_type READ_WRITE = 4;
};

struct TaskInfo
{
	class GameInstance* GameInstance = nullptr;
};

struct TaskDependency
{
	GTSL::Id64 AccessedObject;
	AccessType Access;
};

template<typename TASK, class ALLOCATOR>
struct Goal
{
	Goal() = default;

	Goal(uint32 num, const ALLOCATOR& allocatorReference) :
	taskAccessedObjects(num, allocatorReference),
	taskAccessTypes(num, allocatorReference),
	taskGoalIndex(num, allocatorReference),
	taskNames(num, allocatorReference),
	tasks(num, allocatorReference)
	{
	}

	template<class OALLOC>
	Goal(const Goal<TASK, OALLOC>& other, const ALLOCATOR& allocatorReference) :
	taskAccessedObjects(other.taskAccessedObjects.GetCapacity(), allocatorReference),
	taskAccessTypes(other.taskAccessTypes.GetCapacity(), allocatorReference),
	taskGoalIndex(other.taskGoalIndex, allocatorReference),
	taskNames(other.taskNames, allocatorReference),
	tasks(other.tasks, allocatorReference)
	{
		for(uint32 i = 0; i < other.taskAccessedObjects.GetLength(); ++i)
		{
			taskAccessedObjects.EmplaceBack(other.taskAccessedObjects[i], allocatorReference);
			taskAccessTypes.EmplaceBack(other.taskAccessTypes[i], allocatorReference);
		}
	}

	template<class OALLOC>
	Goal& operator=(const Goal<TASK, OALLOC>& other)
	{
		taskAccessedObjects = other.taskAccessedObjects;
		taskAccessTypes = other.taskAccessTypes;
		taskGoalIndex = other.taskGoalIndex;
		taskNames = other.taskNames;
		tasks = other.tasks;
		return *this;
	}
	
	void AddTask(Id name, TASK task, GTSL::Ranger<const uint16> offsets, const GTSL::Ranger<const AccessType> accessTypes, uint16 goalIndex, const ALLOCATOR& allocator)
	{
		auto task_n = taskAccessedObjects.EmplaceBack(16, allocator);
		taskAccessTypes.EmplaceBack(16, allocator);
		
		taskAccessedObjects.back().PushBack(offsets);
		taskAccessTypes.back().PushBack(accessTypes);
		
		taskNames.EmplaceBack(name);
		taskGoalIndex.EmplaceBack(goalIndex);
		tasks.EmplaceBack(task);
	}

	template<class ALLOC>
	void AddTask(const Goal<TASK, ALLOC>& other, const uint16 taskS, const uint16 taskE, const ALLOCATOR& allocator)
	{
		for (uint32 i = taskS; i < taskE; ++i)
		{
			taskAccessedObjects.EmplaceBack(other.taskAccessedObjects[i], allocator);
			taskAccessTypes.EmplaceBack(other.taskAccessTypes[i], allocator);
		}
		
		taskNames.PushBack(GTSL::Ranger<const Id>(taskE - taskS, other.taskNames.begin() + taskS));
		taskGoalIndex.PushBack(GTSL::Ranger<const uint16>(taskE - taskS, other.taskGoalIndex.begin() + taskS));
		tasks.PushBack(GTSL::Ranger<const TASK>(taskE - taskS, other.tasks.begin() + taskS));
	}

	void RemoveTask(const GTSL::Id64 name)
	{
		auto res = taskNames.Find(name);
		BE_ASSERT(res != taskNames.end(), "No task by that name");

		uint32 i = static_cast<uint32>(res - taskNames.begin());
		
		taskAccessedObjects.Pop(i);
		taskAccessTypes.Pop(i);
		taskGoalIndex.Pop(i);
		taskNames.Pop(i);
		tasks.Pop(i);
	}

	TASK GetTask(const uint32 i) { return tasks[i]; }

	GTSL::Ranger<const uint16> GetTaskAccessedObjects(uint16 task) { return taskAccessedObjects[task]; }

	GTSL::Ranger<const AccessType> GetTaskAccessTypes(uint16 task) { return taskAccessTypes[task]; }

	Id GetTaskName(const uint16 task) const { return taskNames[task]; }
	
	uint16 GetNumberOfTasks() { return static_cast<uint16>(tasks.GetLength()); }
	
	uint16 GetTaskGoalIndex(const uint16 task) { return taskGoalIndex[task]; }

	void Clear()
	{
		for (auto& e : taskAccessedObjects) { e.ResizeDown(0); }
		taskAccessedObjects.ResizeDown(0);
		for (auto& e : taskAccessTypes) { e.ResizeDown(0); }
		taskAccessTypes.ResizeDown(0);

		taskGoalIndex.ResizeDown(0);

		taskNames.ResizeDown(0);
		tasks.ResizeDown(0);
	}
	
private:
	GTSL::Vector<GTSL::Vector<uint16, ALLOCATOR>, ALLOCATOR> taskAccessedObjects;
	GTSL::Vector<GTSL::Vector<AccessType, ALLOCATOR>, ALLOCATOR> taskAccessTypes;
	
	GTSL::Vector<uint16, ALLOCATOR> taskGoalIndex;
	
	GTSL::Vector<Id, ALLOCATOR> taskNames;
	GTSL::Vector<TASK, ALLOCATOR> tasks;

	friend struct Goal;
};

template<class ALLOCATOR>
struct TaskSorter
{
	explicit TaskSorter(const uint32 num, const ALLOCATOR& allocator) :
	currentObjectAccessState(num, allocator), currentObjectAccessCount(num, allocator),
	ongoingTasksAccesses(num, allocator), ongoingTasksObjects(num, allocator)
	{
		currentObjectAccessState.Resize(num);
		for (auto& e : currentObjectAccessState) { e = 0; }
		currentObjectAccessCount.Resize(num);
		for (auto& e : currentObjectAccessCount) { e = 0; }
	}

	GTSL::Result<uint32> CanRunTask(const GTSL::Ranger<const uint16> objects, const GTSL::Ranger<const AccessType> accesses)
	{
		BE_ASSERT(objects.ElementCount() == accesses.ElementCount(), "Bad data, shold be equal");

		const auto elementCount = objects.ElementCount();
		
		{
			GTSL::ReadLock lock(mutex);
			
			for (uint32 i = 0; i < elementCount; ++i)
			{
				if (currentObjectAccessState[objects[i]] == AccessType::READ_WRITE) { return false; }
				if (currentObjectAccessState[objects[i]] == AccessType::READ && accesses[i] == AccessType::READ_WRITE) { return false; }
			}
		}

		{
			GTSL::WriteLock lock(mutex);
			
			for (uint32 i = 0; i < elementCount; ++i)
			{
				currentObjectAccessState[objects[i]] = accesses[i];
				++currentObjectAccessCount[objects[i]];
			}
			
			auto i = ongoingTasksAccesses.EmplaceBack(accesses);
			auto j = ongoingTasksObjects.EmplaceBack(objects);

			BE_ASSERT(i == j, "Error")
			return GTSL::Result<uint32>(GTSL::MoveRef(i), true);
		}
	}

	void ReleaseResources(const uint32 i)
	{
		GTSL::WriteLock lock(mutex);

		ongoingTasksAccesses.Pop(i);
		ongoingTasksObjects.Pop(i);
	}
	
private:
	GTSL::Vector<AccessType::value_type, ALLOCATOR> currentObjectAccessState;
	GTSL::Vector<uint16, ALLOCATOR> currentObjectAccessCount;

	//TODO: should be a keep vector so as not to move data when popping elements
	GTSL::Vector<GTSL::Array<AccessType, 64>, ALLOCATOR> ongoingTasksAccesses;
	GTSL::Vector<GTSL::Array<uint16, 64>, ALLOCATOR> ongoingTasksObjects;

	GTSL::ReadWriteMutex mutex;
};