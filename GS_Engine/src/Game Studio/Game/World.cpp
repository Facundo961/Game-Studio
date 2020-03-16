#include "World.h"
#include "Application/Application.h"


World::World()
{
}

World::~World()
{
	for(auto& e : types)
	{
		delete e.second;
	}
}

void World::OnUpdate()
{
	levelRunningTime += GS::Application::Get()->GetClock().GetDeltaTime();
	levelAdjustedRunningTime += GS::Application::Get()->GetClock().GetDeltaTime() * worldTimeMultiplier;

	for(auto& e : types)
	{
		TypeManager::UpdateInstancesInfo update_instances_info;
		e.second->UpdateInstances(update_instances_info);
	}
}

void World::Pause()
{
	worldTimeMultiplier = 0;
}

double World::GetRealRunningTime() { return GS::Application::Get()->GetClock().GetElapsedTime(); }

float World::GetWorldDeltaTime() const { return static_cast<float>(GS::Application::Get()->GetClock().GetDeltaTime() * worldTimeMultiplier); }
