#pragma once

#include <GTSL/Delegate.hpp>
#include <GTSL/FlatHashMap.h>
#include <GTSL/Id.h>
#include <GTSL/Mutex.h>
#include <GTSL/Vector.hpp>
#include <GTSL/Algorithm.h>
#include <GTSL/Allocator.h>
#include <GTSL/Array.hpp>
#include <GTSL/Pair.h>
#include <GTSL/Semaphore.h>

#include "Tasks.h"
#include "ByteEngine/Id.h"

#include "ByteEngine/Debug/Assert.h"

class World;
class ComponentCollection;
class System;

namespace BE {
	class Application;
}

template<typename... ARGS>
using Task = GTSL::Delegate<void(TaskInfo, ARGS...)>;

inline const char* AccessTypeToString(const AccessType access)
{
	switch (access)
	{
	case AccessType::READ: return "READ";
	case AccessType::READ_WRITE: return "READ_WRITE";
	}
}

class GameInstance : public Object
{
	using FunctionType = GTSL::Delegate<void(GameInstance*, uint32, uint32, uint32)>;
public:
	GameInstance();
	~GameInstance();
	
	void OnUpdate(BE::Application* application);

	using WorldReference = uint8;
	
	struct CreateNewWorldInfo
	{
	};
	template<typename T>
	WorldReference CreateNewWorld(const CreateNewWorldInfo& createNewWorldInfo)
	{
		auto index = worlds.EmplaceBack(GTSL::SmartPointer<World, BE::PersistentAllocatorReference>::Create<T>(GetPersistentAllocator()));
		initWorld(index); return index;
	}

	void UnloadWorld(WorldReference worldId);
	
	template<class T>
	T* GetSystem(const Id systemName)
	{
		GTSL::ReadLock lock(systemsMutex);
		return static_cast<T*>(systemsMap.At(systemName));
	}
	
	template<class T>
	T* GetSystem(const uint16 systemReference)
	{
		GTSL::ReadLock lock(systemsMutex);
		return static_cast<T*>(systems[systemReference].GetData());
	}
	
	uint16 GetSystemReference(const Id systemName)
	{
		GTSL::ReadLock lock(systemsMutex);
		return static_cast<uint16>(systemsIndirectionTable.At(systemName));
	}

	template<typename... ARGS>
	void AddTask(const Id name, const GTSL::Delegate<void(TaskInfo, ARGS...)>& function, const GTSL::Range<const TaskDependency*> dependencies, const Id startOn, const Id doneFor, ARGS&&... args)
	{
		if constexpr (_DEBUG) { if (assertTask(name, startOn, doneFor, dependencies)) { return; } }
		
		auto taskInfo = GTSL::SmartPointer<void*, BE::PersistentAllocatorReference>::Create<DispatchTaskInfo<TaskInfo, ARGS...>>(GetPersistentAllocator(), function, TaskInfo(), GTSL::ForwardRef<ARGS>(args)...);

		auto task = [](GameInstance* gameInstance, const uint32 goal, const uint32 goalTaskIndex, const uint32 dynamicTaskIndex) -> void
		{			
			{
				DispatchTaskInfo<TaskInfo, ARGS...>* info;

				{
					GTSL::ReadLock lock(gameInstance->recurringTasksInfoMutex);
					info = reinterpret_cast<DispatchTaskInfo<TaskInfo, ARGS...>*>(gameInstance->recurringTasksInfo[goal][goalTaskIndex].GetData());
				}
				
				GTSL::Get<0>(info->Arguments).GameInstance = gameInstance;
				GTSL::Call(info->Delegate, info->Arguments);
			}

			gameInstance->resourcesUpdated.NotifyAll();
			gameInstance->semaphores[goal].Post();
			gameInstance->taskSorter.ReleaseResources(dynamicTaskIndex);
		};

		GTSL::Array<uint16, 32> objects; GTSL::Array<AccessType, 32> accesses;

		uint16 startOnGoalIndex, taskObjectiveIndex;

		{
			GTSL::ReadLock lock(goalNamesMutex);
			decomposeTaskDescriptor(dependencies, objects, accesses);
			startOnGoalIndex = getGoalIndex(startOn);
			taskObjectiveIndex = getGoalIndex(doneFor);
		}

		{
			GTSL::WriteLock lock(recurringTasksInfoMutex);
			GTSL::WriteLock lock2(recurringGoalsMutex);
			recurringGoals[startOnGoalIndex].AddTask(name, FunctionType::Create(task), objects, accesses, taskObjectiveIndex, GetPersistentAllocator());
			recurringTasksInfo[startOnGoalIndex].EmplaceBack(GTSL::MoveRef(taskInfo));
		}

		BE_LOG_MESSAGE("Added recurring task ", name.GetString(), " to goal ", startOn.GetString(), " to be done before ", doneFor.GetString())
	}
	
	void RemoveTask(Id name, Id startOn);

	template<typename... ARGS>
	void AddDynamicTask(const Id name, const GTSL::Delegate<void(TaskInfo, ARGS...)>& function, const GTSL::Range<const TaskDependency*> dependencies, const Id startOn, const Id doneFor, ARGS&&... args)
	{
		auto* taskInfo = GTSL::New<DispatchTaskInfo<TaskInfo, ARGS...>>(GetPersistentAllocator(), function, TaskInfo(), GTSL::ForwardRef<ARGS>(args)...);

		GTSL::Array<uint16, 32> objects; GTSL::Array<AccessType, 32> accesses;

		uint16 startOnGoalIndex, taskObjectiveIndex;

		auto task = [](GameInstance* gameInstance, const uint32 goal, const uint32 goalTaskIndex, const uint32 dynamicTaskIndex) -> void
		{
			{
				DispatchTaskInfo<TaskInfo, ARGS...>* info;

				{
					GTSL::ReadLock lock(gameInstance->dynamicTasksInfoMutex);
					info = static_cast<DispatchTaskInfo<TaskInfo, ARGS...>*>(gameInstance->dynamicTasksInfo[goal][goalTaskIndex]);
				}

				GTSL::Get<0>(info->Arguments).GameInstance = gameInstance;
				GTSL::Call(info->Delegate, info->Arguments);

				gameInstance->resourcesUpdated.NotifyAll();
				gameInstance->semaphores[goal].Post();
				GTSL::Delete<DispatchTaskInfo<TaskInfo, ARGS...>>(info, gameInstance->GetPersistentAllocator());
			}

			gameInstance->taskSorter.ReleaseResources(dynamicTaskIndex);
		};

		{
			GTSL::ReadLock lock(goalNamesMutex);
			decomposeTaskDescriptor(dependencies, objects, accesses);
			startOnGoalIndex = getGoalIndex(startOn);
			taskObjectiveIndex = getGoalIndex(doneFor);
		}
		
		{
			GTSL::WriteLock lock(dynamicTasksInfoMutex);
			GTSL::WriteLock lock2(dynamicGoalsMutex);
			dynamicGoals[startOnGoalIndex].AddTask(name, FunctionType::Create(task), objects, accesses, taskObjectiveIndex, GetPersistentAllocator());
			dynamicTasksInfo[startOnGoalIndex].EmplaceBack(taskInfo);
		}

		BE_LOG_MESSAGE("Added dynamic task ", name.GetString(), " to goal ", startOn.GetString(), " to be done before ", doneFor.GetString())
	}

	template<typename... ARGS>
	void AddDynamicTask(const Id name, const GTSL::Delegate<void(TaskInfo, ARGS...)>& function, const GTSL::Range<const TaskDependency*> dependencies, ARGS&&... args)
	{
		auto task = [](GameInstance* gameInstance, const uint32 goal, const uint32 indexIntoTasksArray, const uint32 asyncTasksIndex) -> void
		{
			{
				DispatchTaskInfo<TaskInfo, ARGS...>* info;

				{
					GTSL::ReadLock lock(gameInstance->asyncTasksInfoMutex);
					info = static_cast<DispatchTaskInfo<TaskInfo, ARGS...>*>(gameInstance->asyncTasksInfo[indexIntoTasksArray]);
				}

				GTSL::Get<0>(info->Arguments).GameInstance = gameInstance;
				GTSL::Call(info->Delegate, info->Arguments);
				GTSL::Delete<DispatchTaskInfo<TaskInfo, ARGS...>>(info, gameInstance->GetPersistentAllocator());
			}

			gameInstance->resourcesUpdated.NotifyAll();
			gameInstance->taskSorter.ReleaseResources(asyncTasksIndex);
		};

		GTSL::Array<uint16, 32> objects; GTSL::Array<AccessType, 32> accesses;

		{
			GTSL::ReadLock lock(goalNamesMutex);
			decomposeTaskDescriptor(dependencies, objects, accesses);
		}

		{
			GTSL::WriteLock lock(asyncTasksMutex);
			asyncTasks.AddTask(name, FunctionType::Create(task), objects, accesses, 0xFFFFFFFF, GetPersistentAllocator());
		}

		{
			GTSL::WriteLock lock(asyncTasksInfoMutex);
			auto* taskInfo = GTSL::New<DispatchTaskInfo<TaskInfo, ARGS...>>(GetPersistentAllocator(), function, TaskInfo(), GTSL::ForwardRef<ARGS>(args)...);
			taskInfo->TaskIndex = asyncTasksInfo.EmplaceBack(taskInfo);
		}

		BE_LOG_MESSAGE("Added free dynamic task ", name.GetString())
	}

	void AddGoal(Id name);

private:
	GTSL::Vector<GTSL::SmartPointer<World, BE::PersistentAllocatorReference>, BE::PersistentAllocatorReference> worlds;
	
	mutable GTSL::ReadWriteMutex systemsMutex;
	GTSL::KeepVector<GTSL::SmartPointer<System, BE::PersistentAllocatorReference>, BE::PersistentAllocatorReference> systems;
	GTSL::KeepVector<Id, BE::PersistentAllocatorReference> systemNames;
	GTSL::FlatHashMap<System*, BE::PersistentAllocatorReference> systemsMap;
	GTSL::FlatHashMap<uint32, BE::PersistentAllocatorReference> systemsIndirectionTable;
	
	template<typename... ARGS>
	struct DispatchTaskInfo
	{
		DispatchTaskInfo(const GTSL::Delegate<void(ARGS...)>& delegate, ARGS&&... args) : Delegate(delegate), Arguments(GTSL::ForwardRef<ARGS>(args)...)
		{
		}

		uint32 TaskIndex;
		GTSL::Delegate<void(ARGS...)> Delegate;
		GTSL::Tuple<ARGS...> Arguments;
	};
	
	mutable GTSL::ReadWriteMutex recurringGoalsMutex;
	GTSL::Vector<Goal<FunctionType, BE::PersistentAllocatorReference>, BE::PersistentAllocatorReference> recurringGoals;
	mutable GTSL::ReadWriteMutex dynamicGoalsMutex;
	GTSL::Vector<Goal<FunctionType, BE::PersistentAllocatorReference>, BE::PersistentAllocatorReference> dynamicGoals;
	
	mutable GTSL::ReadWriteMutex asyncTasksMutex;
	Goal<FunctionType, BE::PersistentAllocatorReference> asyncTasks;
	mutable GTSL::ReadWriteMutex asyncTasksInfoMutex;
	GTSL::Vector<void*, BE::PersistentAllocatorReference> asyncTasksInfo;

	GTSL::ConditionVariable resourcesUpdated;
	
	mutable GTSL::ReadWriteMutex goalNamesMutex;
	GTSL::Vector<Id, BE::PersistentAllocatorReference> goalNames;

	mutable GTSL::ReadWriteMutex recurringTasksInfoMutex;
	GTSL::Vector<GTSL::Vector<GTSL::SmartPointer<void*, BE::PersistentAllocatorReference>, BE::PersistentAllocatorReference>, BE::PersistentAllocatorReference> recurringTasksInfo;
	
	mutable GTSL::ReadWriteMutex dynamicTasksInfoMutex;
	GTSL::Vector<GTSL::Vector<void*, BE::PersistentAllocatorReference>, BE::PersistentAllocatorReference> dynamicTasksInfo;

	TaskSorter<BE::PersistentAllocatorReference> taskSorter;

	mutable GTSL::ReadWriteMutex functionDependenciesMutex;
	GTSL::KeepVector<GTSL::Pair<GTSL::Array<uint16, 32>, GTSL::Array<AccessType, 32>>, BE::PAR> functionDependencies;
	
	GTSL::Vector<GTSL::Semaphore, BE::PAR> semaphores;

	uint32 scalingFactor = 16;

	uint64 frameNumber = 0;

	GTSL::StaticString<1024> genTaskLog(const char* from, Id taskName, const GTSL::Range<const AccessType*> accesses, const GTSL::Range<const uint16*> objects)
	{
		GTSL::StaticString<1024> log;

		log += from;
		log += taskName.GetString();

		log += '\n';
		
		log += "Accessed objects: \n	";
		for (uint16 i = 0; i < objects.ElementCount(); ++i)
		{
			log += "Obj: "; log += systemNames[objects[i]].GetString(); log += ". Access: "; log += AccessTypeToString(accesses[i]); log += "\n	";
		}

		return log;
	}
	
	GTSL::StaticString<1024> genTaskLog(const char* from, Id taskName, Id goalName, const GTSL::Range<const AccessType*> accesses, const GTSL::Range<const uint16*> objects)
	{
		GTSL::StaticString<1024> log;

		log += from;
		log += taskName.GetString();

		log += '\n';

		log += " Goal: ";
		log += goalName.GetString();

		log += '\n';
		
		log += "Accessed objects: \n	";
		for (uint16 i = 0; i < objects.ElementCount(); ++i)
		{
			log += "Obj: "; log += systemNames[objects[i]].GetString(); log += ". Access: "; log += AccessTypeToString(accesses[i]); log += "\n	";
		}

		return log;
	}

	uint16 getGoalIndex(const Id name) const
	{
		uint16 i = 0; for (auto goal_name : goalNames) { if (goal_name == name) break; ++i; }
		BE_ASSERT(i != goalNames.GetLength(), "No goal found with that name!")
		return i;
	}
	
	template<uint32 N>
	void decomposeTaskDescriptor(GTSL::Range<const TaskDependency*> taskDependencies, GTSL::Array<uint16, N>& object, GTSL::Array<AccessType, N>& access)
	{
		object.Resize(taskDependencies.ElementCount()); access.Resize(taskDependencies.ElementCount());
		
		for (uint16 i = 0; i < static_cast<uint16>(taskDependencies.ElementCount()); ++i) //for each dependency
		{
			object[i] = systemsIndirectionTable.At(taskDependencies[i].AccessedObject);
			access[i] = (taskDependencies.begin() + i)->Access;
		}
	}

	[[nodiscard]] bool assertTask(const Id name, const Id startGoal, const Id endGoal, const GTSL::Range<const TaskDependency*> dependencies) const
	{
		{
			GTSL::ReadLock lock(goalNamesMutex);
			
			if (goalNames.Find(startGoal) == goalNames.end())
			{
				BE_LOG_WARNING("Tried to add task ", name.GetString(), " to goal ", startGoal.GetString(), " which doesn't exist. Resolve this issue as it leads to undefined behavior in release builds!")
				return true;
			}

			//assert done for exists
			if (goalNames.Find(endGoal) == goalNames.end())
			{
				BE_LOG_WARNING("Tried to add task ", name.GetString(), " ending for goal ", endGoal.GetString(), " which doesn't exist. Resolve this issue as it leads to undefined behavior in release builds!")
				return true;
			}
		}

		{
			GTSL::ReadLock lock(recurringGoalsMutex);
			
			if (recurringGoals[getGoalIndex(startGoal)].DoesTaskExist(name))
			{
				BE_LOG_WARNING("Tried to add task ", name.GetString(), " which already exists to goal ", startGoal.GetString(), ". Resolve this issue as it leads to undefined behavior in release builds!")
				return true;
			}
		}

		{
			GTSL::ReadLock lock(systemsMutex);

			for(auto e : dependencies)
			{
				if (!systemsMap.Find(e.AccessedObject)) {
					BE_LOG_ERROR("Tried to add task ", name.GetString(), " to goal ", startGoal.GetString(), " with a dependency on ", e.AccessedObject.GetString(), " which doesn't exist. Resolve this issue as it leads to undefined behavior in release builds!")
					return true;
				}
			}
		}

		return false;
	}

	void initWorld(uint8 worldId);
	void initSystem(System* system, GTSL::Id64 name, const uint16 id);
	
public:
	template<typename T>
	T* AddSystem(const Id systemName)
	{
		System* system;

		uint32 l;
		
		{
			GTSL::WriteLock lock(systemsMutex);
			l = systems.Emplace(GTSL::SmartPointer<System, BE::PersistentAllocatorReference>::Create<T>(GetPersistentAllocator()));
			systemsMap.Emplace(systemName, systems[l]);
			systemsIndirectionTable.Emplace(systemName, l);
			systemNames.Emplace(systemName);
			system = systems[l];
		}

		initSystem(system, systemName, static_cast<uint16>(l));

		taskSorter.AddSystem(systemName);
		
		return static_cast<T*>(system);
	}
};
