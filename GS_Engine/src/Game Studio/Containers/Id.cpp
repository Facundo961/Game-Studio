#include "Id.h"

#include "FString.h"

Id::Id(const char * Text): HashedString(HashString(Text))
{
}

uint32 Id::HashString(const char * Text)
{
	const uint32 Length = FString::StringLength(Text);

	uint32 Hash = 0;

	for (uint32 i = 0; i < Length; i++)
	{
		Hash += Text[i] * i * 33;
	}

	Hash = Hash * Length * 5;

	return Hash;
}