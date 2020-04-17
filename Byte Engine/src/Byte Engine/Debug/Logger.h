#pragma once

class Object;

#undef ERROR

enum class LogLevel
{
	MESSAGE,
	SUCCESS,
	WARNING,
	FATAL
};

class Logger
{
	static LogLevel MinLogLevel;

	static void SetTextColorOnLogLevel(LogLevel _Level);
public:
	static void PrintObjectLog(const Object* obj, LogLevel level, const char* text, ...);
	static void PrintBasicLog(LogLevel _Level, const char* Text, ...);
	static void SetMinLogLevel(const LogLevel _Level) { MinLogLevel = _Level; }

#ifdef BE_DEBUG

#define BE_LOG_SUCCESS(Text, ...)	Logger::PrintObjectLog(this, LogLevel::SUCCESS, Text, __VA_ARGS__);
#define BE_LOG_MESSAGE(Text, ...)	Logger::PrintObjectLog(this, LogLevel::MESSAGE, Text, __VA_ARGS__);
#define BE_LOG_WARNING(Text, ...)	Logger::PrintObjectLog(this, LogLevel::WARNING, Text, __VA_ARGS__);
#define BE_LOG_ERROR(Text, ...)		Logger::PrintObjectLog(this, LogLevel::FATAL, Text, __VA_ARGS__);
#define BE_LOG_LEVEL(_Level)		Logger::SetMinLogLevel(_Level);

#define BE_BASIC_LOG_SUCCESS(Text, ...)	Logger::PrintBasicLog(LogLevel::SUCCESS, Text, __VA_ARGS__);
#define BE_BASIC_LOG_MESSAGE(Text, ...)	Logger::PrintBasicLog(LogLevel::MESSAGE, Text, __VA_ARGS__);
#define BE_BASIC_LOG_WARNING(Text, ...)	Logger::PrintBasicLog(LogLevel::WARNING, Text, __VA_ARGS__);
#define BE_BASIC_LOG_ERROR(Text, ...)	Logger::PrintBasicLog(LogLevel::FATAL, Text, __VA_ARGS__);
#else
#define BE_LOG_SUCCESS(Text, ...)
#define BE_LOG_MESSAGE(Text, ...)
#define BE_LOG_WARNING(Text, ...)
#define BE_LOG_ERROR(Text, ...)
#define BE_LOG_LEVEL(_Level)
#define BE_BASIC_LOG_SUCCESS(Text, ...)	
#define BE_BASIC_LOG_MESSAGE(Text, ...)	
#define BE_BASIC_LOG_WARNING(Text, ...)	
#define BE_BASIC_LOG_ERROR(Text, ...)	
#endif
};
