#include "EventDispatcher.h"

#include "Event.h"

EventDispatcher::EventDispatcher() : SubscriberInfo(50), EventQueue(50)
{
}

EventDispatcher::~EventDispatcher()
{
}

void EventDispatcher::OnUpdate()
{
	//For every element inside of the event queue.
	for (unsigned short i = 0; i < EventQueue.length(); i++)
	{
		//Access SubscriberInfo[(at the current event queue's EventId)] and loop through each calling the function whith the current event queue event as a parameter.
		for (unsigned short j = 0; j < SubscriberInfo[i].length(); j++)
		{
			//SubscriberInfo at 
			SubscriberInfo[i][j](*EventQueue[i]);
		}

		delete EventQueue[i];
	}
}

uint16 EventDispatcher::CreateEvent()
{
	EventCount++;

	return EventCount;
}

void EventDispatcher::Subscribe(unsigned short EventId, Object * Subscriber, MemberFuncPtr Func)
{
	//unsigned short EventIndex = Loop(EventId);		//Call loop and store the return in _local_var_EventIndex.

	SubscriberInfo[EventId].push_back(Functor(Subscriber, Func));			//Access SubscriberInfo at _local_var_EventIndex and store in the array inside that index the function to call.

	return;
}

void EventDispatcher::UnSubscribe(unsigned short EventId, Object * Subscriber)
{
	for (unsigned short i = 0; i < SubscriberInfo[EventId].length(); i++)
	{
		if (SubscriberInfo[EventId][i].Obj == Subscriber)
		{
			SubscriberInfo[EventId].erase(i);
		}
	}
}