#pragma once

#include "Core.h"

#include "EngineSystem.h"

#include "Event.h"

#include "DArray.hpp"

typedef void (*FunctionPointer)(const Event & Event);

struct Func
{
	Func(FunctionPointer Pointer) : FuncPointer(Pointer)
	{
	}

	FunctionPointer FuncPointer = nullptr;
};

GS_CLASS EventDispatcher : public ESystem
{
public:
	EventDispatcher();
	~EventDispatcher();

	void OnUpdate() override;

	static unsigned short CreateEvent();
	static void Subscribe(unsigned short EventId, FunctionPointer FunctionToCall);
	static void UnSubscribe(unsigned short EventId, FunctionPointer OrigFunction);
	static void Notify(unsigned short Index, Event & Event);

private:
	//Determines which levels receive the events. Every level from the specified level upwards will get the events.
	static unsigned char					ActiveLevel;

	//static SArray<unsigned short>			Events;
	static unsigned short						EventCount;
	static DArray<DArray<FunctionPointer>>		EventInfo;
	static DArray<Event>						EventQueue;

	void Dispatch(unsigned short Index);
	static int Loop(unsigned short EventId);
};