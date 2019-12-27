#include "Matrix4.h"

#include "SIMD/float4.h"

Vector4 Matrix4::operator*(const Vector4& Other) const
{
	alignas(16) Vector4 Result;

	const auto P1(float4::MakeFromUnaligned(&Other.X) * float4::MakeFromUnaligned(&Array[0]));
	const auto P2(float4::MakeFromUnaligned(&Other.Y) * float4::MakeFromUnaligned(&Array[4]));
	const auto P3(float4::MakeFromUnaligned(&Other.Z) * float4::MakeFromUnaligned(&Array[8]));
	const auto P4(float4::MakeFromUnaligned(&Other.W) * float4::MakeFromUnaligned(&Array[12]));

	const float4 res = P1 + P2 + P3 + P4;

	res.CopyToAlignedData(&Result.X);

	return Result;
}

Matrix4 Matrix4::operator*(const Matrix4& Other) const
{
	Matrix4 Result;

	auto Row1 = float4::MakeFromUnaligned(&Other.Array[0]);
	auto Row2 = float4::MakeFromUnaligned(&Other.Array[4]);
	auto Row3 = float4::MakeFromUnaligned(&Other.Array[8]);
	auto Row4 = float4::MakeFromUnaligned(&Other.Array[12]);

	float4 Bro1;
	float4 Bro2;
	float4 Bro3;
	float4 Bro4;

	float4 Row;

	for (uint8 i = 0; i < 4; ++i)
	{
		Bro1 = Array[4 * i + 0];
		Bro2 = Array[4 * i + 1];
		Bro3 = Array[4 * i + 2];
		Bro4 = Array[4 * i + 3];

		Row = ((Bro1 * Row1) + (Bro2 * Row2)) + ((Bro3 * Row3) + (Bro4 * Row4));

		Row.CopyToAlignedData(&Result.Array[4 * i]);
	}

	return Result;
}

Matrix4& Matrix4::operator*=(const float Other)
{
	float Input = Other;
	const __m512 InputVector = _mm512_set1_ps(Input);
	const __m512 MatrixVector = _mm512_load_ps(Array);

	const __m512 Result = _mm512_mul_ps(InputVector, MatrixVector);

	_mm512_storeu_ps(Array, Result);

	return *this;
}

Matrix4& Matrix4::operator*=(const Matrix4& Other)
{
	auto Row1 = float4::MakeFromUnaligned(&Other.Array[0]);
	auto Row2 = float4::MakeFromUnaligned(&Other.Array[4]);
	auto Row3 = float4::MakeFromUnaligned(&Other.Array[8]);
	auto Row4 = float4::MakeFromUnaligned(&Other.Array[12]);

	float4 brod1;
	float4 brod2;
	float4 brod3;
	float4 brod4;

	float4 Row;

	for (uint8 i = 0; i < 4; ++i)
	{
		brod1 = Array[4 * i + 0];
		brod2 = Array[4 * i + 1];
		brod3 = Array[4 * i + 2];
		brod4 = Array[4 * i + 3];

		Row = (brod1 * Row1) + (brod2 * Row2) + (brod3 * Row3) + (brod4 * Row4);

		Row.CopyToAlignedData(&Array[4 * i]);
	}

	return *this;
}
