#include "FString.h"
#include <cstdio>
#include <stdarg.h>

FString::FString() : Data(10)
{
}

FString::FString(const char * In) : Data(StringLength(In), const_cast<char*>(In))
{
}

FString::FString(char* const _In) : Data(StringLength(_In), CCAST(char*, _In))
{
}

FString::FString(size_t _Length) : Data(_Length)
{
}

FString::FString(const size_t Length, const char* In) : Data(Length + 1, const_cast<char*>(In))
{
	Data.push_back('\0');
}

FString & FString::operator=(const char * In)
{
	Data.recreate(const_cast<char *>(In), StringLength(In));

	return *this;
}

FString & FString::operator+(const char * Other)
{
	Data.push_back(StringLength(Other), const_cast<char*>(Other));

	return *this;
}

FString & FString::operator+(const FString & Other)
{
	Data.push_back(Other.Data);

	return *this;
}

char FString::operator[](size_t _Index)
{
	return Data[_Index];
}

char FString::operator[](size_t _Index) const
{
	return Data[_Index];
}

bool FString::operator==(const FString & _Other) const
{
	if (Data.length() != _Other.Data.length())
	{
		return false;
	}

	for (size_t i = 0; i < Data.length(); i++)
	{
		if(Data[i] != _Other.Data[i])
		{
			return false;
		}
	}

	return true;
}

bool FString::NonSensitiveComp(const FString& _Other) const
{
	if (Data.length() != _Other.Data.length())
	{
		return false;
	}

	for (size_t i = 0; i < Data.length(); i++)
	{
		if (Data[i] != ((ToLowerCase(_Other.Data[i]) | ToUpperCase(_Other.Data[i])) != 0))
		{
			return false;
		}
	}
}

char * FString::c_str()
{
	return Data.data();
}

const char * FString::c_str() const
{
	return Data.data();
}

void FString::Append(const char * In)
{
	Data.push_back(' ');

	Data.push_back(StringLength(In), const_cast<char*>(In));

	return;
}

void FString::Append(const FString & In)
{
	Data.push_back(' ');

	Data.push_back(In.Data);

	return;
}

void FString::Insert(const char * In, const size_t Index)
{
	Data.insert(Index, const_cast<char *>(In), StringLength(In));

	return;
}

int64 FString::FindLast(char _Char) const
{
	for (int32 i = Data.length(); i > 0; --i)
	{
		if (Data[i] == _Char) return i;
	}

	return -1;
}

size_t FString::StringLength(const char * In)
{
	size_t Length = 0;

	while(In[Length] != '\0')
	{
		Length++;
	}

	//We return Length + 1 to take into account for the null terminator character.
	return Length + 1;
}

#define FSTRING_MAKESTRING_DEFAULT_SIZE 256

FString FString::MakeString(const char* _Text, ...)
{
	FString Return(FSTRING_MAKESTRING_DEFAULT_SIZE);

	va_list vaargs;
	va_start(vaargs, _Text);
	const auto Count = snprintf(Return.Data.data(), Return.Data.length(), _Text, vaargs) + 1; //Take into account null terminator.
	if(Count > Return.Data.length())
    {
        Return.Data.resize(Count);

        snprintf(Return.Data.data(), Return.Data.length(), _Text, vaargs);
    }
	va_end(vaargs);

	return Return;
}

char FString::ToLowerCase(char _Char)
{
	if ('A' <= _Char && _Char <= 'Z')
		return _Char += ('a' - 'A');
	return 0;
}

char FString::ToUpperCase(char _Char)
{
	if ('a' <= _Char && _Char <= 'z')
		return _Char += ('a' - 'A');
	return 0;
}
