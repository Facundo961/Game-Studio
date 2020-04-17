#include "Clock.h"

#ifdef BE_PLATFORM_WIN
#include "windows.h"
#endif

Clock::Clock()
{
#ifdef BE_PLATFORM_WIN
	LARGE_INTEGER WinProcessorFrequency;
	LARGE_INTEGER WinProcessorTicks;

	QueryPerformanceFrequency(&WinProcessorFrequency);
	QueryPerformanceCounter(&WinProcessorTicks);

	processorFrequency = WinProcessorFrequency.QuadPart;
	startPerformanceCounterTicks = WinProcessorTicks.QuadPart;
	performanceCounterTicks = WinProcessorTicks.QuadPart;
#endif
}

Clock::~Clock() = default;

void Clock::OnUpdate()
{
#ifdef BE_PLATFORM_WIN
	LARGE_INTEGER current_ticks;
	QueryPerformanceCounter(&current_ticks);

	//ticks / processsorFrequency = seconds
	
	auto delta_ticks = current_ticks.QuadPart - performanceCounterTicks;	

	auto pc_time_in_microseconds = (performanceCounterTicks * 1000000);
	pc_time_in_microseconds /= processorFrequency;
	
	const auto pc_time = GTSL::TimePoint::CreateFromMicroseconds(pc_time_in_microseconds);
	
	delta_ticks *= 1000000; //to microseconds
	delta_ticks /= processorFrequency;

	//Check if delta_ticks exceeds 1 seconds.
	//This is done to prevent possible problems caused by large time deltas,
	//which could be caused by checking breakpoints during development
	//or by occasional freezes during normal game-play.

	auto delta_microseconds = current_ticks.QuadPart - performanceCounterTicks;
	delta_microseconds *= 1000000;
	delta_microseconds /= processorFrequency;
	auto delta_time = GTSL::TimePoint::CreateFromMicroseconds(delta_microseconds);
	
	if (delta_ticks > 1000000)
	{
		//Leave delta time as is. Assume last frame's delta time.
	}
	else
	{	
	}
	deltaTime = delta_time;
	
	elapsedTime += GTSL::TimePoint::CreateFromMicroseconds(delta_microseconds);

	//Set system ticks as this frame's ticks so in the next update we can work with it.
	performanceCounterTicks = current_ticks.QuadPart;
#endif
	
	//Update elapsed time counter.
	++applicationTicks;
}


#undef GetCurrentTime
GTSL::TimePoint Clock::GetCurrentTime() const
{
#ifdef BE_PLATFORM_WIN
	LARGE_INTEGER win_processor_ticks; QueryPerformanceCounter(&win_processor_ticks); return GTSL::TimePoint::CreateFromMicroseconds(win_processor_ticks.QuadPart * 1000000 / processorFrequency);
#endif
}

//UTILITY GETTERS


uint16 Clock::GetYear()
{
#ifdef BE_PLATFORM_WIN
	SYSTEMTIME WinTimeStructure;
	GetLocalTime(&WinTimeStructure);
	return WinTimeStructure.wYear;
#endif
}

Months Clock::GetMonth()
{
#ifdef BE_PLATFORM_WIN
	SYSTEMTIME WinTimeStructure;
	GetLocalTime(&WinTimeStructure);
	return static_cast<Months>(WinTimeStructure.wMonth);
#endif
}

uint8 Clock::GetDayOfMonth()
{
#ifdef BE_PLATFORM_WIN
	SYSTEMTIME WinTimeStructure;
	GetLocalTime(&WinTimeStructure);
	return WinTimeStructure.wDay;
#endif
}

Days Clock::GetDayOfWeek()
{
#ifdef BE_PLATFORM_WIN
	SYSTEMTIME WinTimeStructure;
	GetLocalTime(&WinTimeStructure);
	return (WinTimeStructure.wDayOfWeek == 0) ? Days::Sunday : static_cast<Days>(WinTimeStructure.wDayOfWeek);
#endif
}

Time Clock::GetTime()
{
#ifdef BE_PLATFORM_WIN
	SYSTEMTIME WinTimeStructure;
	GetLocalTime(&WinTimeStructure);
	return { static_cast<uint8>(WinTimeStructure.wHour), static_cast<uint8>(WinTimeStructure.wMinute), static_cast<uint8>(WinTimeStructure.wSecond) };
#endif
}
