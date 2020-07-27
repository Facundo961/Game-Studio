#include "GameInstance.h"

#include "ComponentCollection.h"

#include "System.h"
#include "ByteEngine/Debug/Assert.h"
#include "ByteEngine/Debug/FunctionTimer.h"

#include <GTSL/Semaphore.h>

#include "ByteEngine/Application/Application.h"
#include "ByteEngine/Application/ThreadPool.h"

using namespace GTSL;

GameInstance::GameInstance() : worlds(4, GetPersistentAllocator()), systems(8, GetPersistentAllocator()), componentCollections(64, GetPersistentAllocator()),
goals(16, GetPersistentAllocator()), goalNames(8, GetPersistentAllocator())
{
}

GameInstance::~GameInstance()
{
	ForEach(systems, [&](SmartPointer<System, BE::PersistentAllocatorReference>& system) { system->Shutdown(); });

	World::DestroyInfo destroy_info;
	destroy_info.GameInstance = this;
	for (auto& world : worlds) { world->DestroyWorld(destroy_info); }
}

void GameInstance::OnUpdate(BE::Application* application)
{
	PROFILE;
	
	TaskSorter<BE::TAR> task_sorter(32, GetTransientAllocator());
	
	Vector<Goal<TaskType, BE::TAR>, BE::TAR> dynamic_goals(32, GetTransientAllocator());
	{
		for(uint32 i = 0; i < dynamic_goals.GetLength(); ++i)
		{
			dynamic_goals.EmplaceBack(64, GetTransientAllocator());
		}
	}
	
	Vector<Vector<Semaphore, BE::TAR>, BE::TAR> semaphores(64, GetTransientAllocator());
	{
		for (auto& e : semaphores)
		{
			e.Initialize(128, GetTransientAllocator());
			for (uint8 i = 0; i < 128; ++i) { e.EmplaceBack(); }
		}
	}
	
	TaskInfo task_info; task_info.GameInstance = this;
	
	uint16 goal_n = 0;
	uint16 goal_number_of_tasks;
	
	for(auto& goal : dynamic_goals)
	{
		goal.GetNumberOfTasks(goal_number_of_tasks);
		
		for (auto& semaphore : semaphores[goal_n]) { semaphore.Wait(); }

		uint16 i = goal_number_of_tasks;
		
		while(i != 0)
		{
			bool can_run = false;
			Ranger<const uint16> accessed_objects; Ranger<const AccessType> access_types;
			goal.GetTaskAccessedObjects(i, accessed_objects); goal.GetTaskAccessTypes(i, access_types);
			
			task_sorter.CanRunTask(can_run, accessed_objects, access_types);

			auto on_done = [](const Array<uint16, 64>& objects, const Array<AccessType, 64>& accesses, decltype(task_sorter)* taskSorter) -> void
			{
				taskSorter->UpdateResources(objects, accesses);
			};

			if (can_run)
			{
				auto on_done_del = Delegate<void(const Array<uint16, 64>&, const Array<AccessType, 64>&, decltype(task_sorter)*)>::Create(on_done);
				Tuple<Array<uint16, 64>, Array<AccessType, 64>, decltype(task_sorter)*> test_args(accessed_objects, access_types, &task_sorter);

				TaskType task; goal.GetTask(task, i);
				application->GetThreadPool()->EnqueueTask(task, on_done_del, &semaphores[goal_n][i], MakeTransferReference(test_args), task_info);
				
				--i;
			}
		}
		
		++goal_n;
	}
}

void GameInstance::AddTask(const Id64 name, const Delegate<void(TaskInfo)> function, const Ranger<const TaskDependency> actsOn, const Id64 startsOn, const Id64 doneFor)
{
	Array<uint16, 32> objects; Array<AccessType, 32> accesses;

	uint16 goal_index = 0;

	{
		ReadLock lock(goalNamesMutex);
		decomposeTaskDescriptor(actsOn, objects, accesses);
		getGoalIndex(startsOn, goal_index);
	}

	goalsMutex.WriteLock();
	goals[goal_index].AddTask(name, function, objects, accesses, doneFor, GetPersistentAllocator());
	goalsMutex.WriteUnlock();
}

void GameInstance::RemoveTask(const Id64 name, const Id64 doneFor)
{
	uint16 i = 0;
	{
		ReadLock lock(goalNamesMutex);
		getGoalIndex(name, i);
	}
	
	goalsMutex.WriteLock();
	goals[i].RemoveTask(name);
	goalsMutex.WriteUnlock();

	BE_ASSERT(false, "No task under specified name!")
}

void GameInstance::AddGoal(Id64 name)
{
	{
		WriteLock lock(goalsMutex);
		goals.EmplaceBack(16, GetPersistentAllocator());
	}

	{
		WriteLock lock(goalNamesMutex);
		goalNames.EmplaceBack(name);
	}
}

void GameInstance::popDynamicTask(DynamicTaskFunctionType& dynamicTaskFunction, uint32& i)
{
	WriteLock lock(newDynamicTasks);
	i = dynamicGoals.GetLength() - 1;
	dynamicGoals.back().GetTask(dynamicTaskFunction, i);
	dynamicGoals.PopBack();
}

void GameInstance::initWorld(const uint8 worldId)
{
	World::InitializeInfo initialize_info;
	initialize_info.GameInstance = this;
	worlds[worldId]->InitializeWorld(initialize_info);
}

void GameInstance::initCollection(ComponentCollection* collection)
{
	//collection->Initialize();
}

void GameInstance::initSystem(System* system, const Id64 name)
{
	System::InitializeInfo initialize_info;
	initialize_info.GameInstance = this;
	system->Initialize(initialize_info);
}