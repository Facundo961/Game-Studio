#include "BEM.hpp"

#include "SIMD/float4.h"

#include <cmath>

float BEM::Power(const float x, const float y) { return powf(x, y); }

float BEM::Log10(const float x) { return log10f(x); }

float BEM::Sine(const float Degrees) { return sinf(DegreesToRadians(Degrees)); }

double BEM::Sine(const double Degrees) { return sin(DegreesToRadians(Degrees)); }

float BEM::Cosine(const float Degrees) { return cosf(DegreesToRadians(Degrees)); }

double BEM::Cosine(const double Degrees) { return cos(DegreesToRadians(Degrees)); }

float BEM::Tangent(const float Degrees) { return tanf(DegreesToRadians(Degrees)); }

double BEM::Tangent(const double Degrees) {	return tan(DegreesToRadians(Degrees)); }

float BEM::ArcSine(const float A) {	return RadiansToDegrees(asin(A)); }

float BEM::ArcCosine(const float A) { return RadiansToDegrees(acos(A)); }

float BEM::ArcTangent(const float A) { return RadiansToDegrees(atan(A)); }

float BEM::ArcTan2(const float X, const float Y) { return RadiansToDegrees(atan2(Y, X)); }

float BEM::LengthSquared(const Vector2& _A)
{
	float4 a(_A.X, _A.Y, 0.0f, 0.0f);
	return float4::DotProduct(a, a).GetX();
}

float BEM::LengthSquared(const Vector3& _A)
{
	float4 a(_A.X, _A.Y, _A.Z, 0.0f);
	return float4::DotProduct(a, a).GetX();
}

float BEM::LengthSquared(const Vector4& _A)
{
	float4 a(_A.X, _A.Y, _A.Z, _A.W);
	return float4::DotProduct(a, a).GetX();
}

Vector2 BEM::Normalized(const Vector2& _A)
{
	float4 a(_A.X, _A.Y, 0.0f, 0.0f);
	const float4 length(Length(_A));
	a /= length;
	alignas(16) float vector[4];
	a.CopyToAlignedData(vector);

	return Vector2(vector[0], vector[1]);
}

void BEM::Normalize(Vector2& _A)
{
	float4 a(_A.X, _A.Y, 0.0f, 0.0f);
	const float4 length(Length(_A));
	a /= length;
	alignas(16) float vector[4];
	a.CopyToAlignedData(vector);
	_A.X = vector[0];
	_A.Y = vector[1];
}

Vector3 BEM::Normalized(const Vector3& _A)
{
	float4 a(_A.X, _A.Y, _A.Z, 0.0f);
	const float4 length(Length(_A));
	a /= length;
	alignas(16) float vector[4];
	a.CopyToAlignedData(vector);

	return Vector3(vector[3], vector[2], vector[1]);
}

void BEM::Normalize(Vector3& _A)
{
	float4 a(_A.X, _A.Y, _A.Z, 0.0f);
	const float4 length(Length(_A));
	a /= length;
	alignas(16) float Vector[4];
	a.CopyToAlignedData(Vector);
	_A.X = Vector[0];
	_A.Y = Vector[1];
	_A.Z = Vector[2];
}

Vector4 BEM::Normalized(const Vector4& _A)
{
	alignas(16) Vector4 result;
	auto a = float4::MakeFromUnaligned(&_A.X);
	const float4 length(Length(_A));
	a /= length;
	a.CopyToAlignedData(&result.X);
	return result;
}

void BEM::Normalize(Vector4& _A)
{
	auto a = float4::MakeFromUnaligned(&_A.X);
	const float4 length(Length(_A));
	a /= length;
	a.CopyToUnalignedData(&_A.X);
}

float BEM::DotProduct(const Vector2& _A, const Vector2& _B)
{
	return float4::DotProduct(float4(_A.X, _A.Y, 0.0f, 0.0f), float4(_B.X, _B.Y, 0.0f, 0.0f)).GetX();
}

float BEM::DotProduct(const Vector3& _A, const Vector3& _B)
{
	return float4::DotProduct(float4(_A.X, _A.Y, _A.Z, 0.0f), float4(_B.X, _B.Y, _B.Z, 0.0f)).GetX();
}

float BEM::DotProduct(const Vector4& _A, const Vector4& _B)
{
	return float4::DotProduct(float4(_A.X, _A.Y, _A.Z, _A.W), float4(_B.X, _B.Y, _B.Z, _A.W)).GetX();
}

Vector3 BEM::Cross(const Vector3& _A, const Vector3& _B)
{
	//alignas(16) float vector[4];
	//
	//const float4 a(_A.X, _A.Y, _A.Z, 1.0f);
	//const float4 b(_B.X, _B.Y, _B.Z, 1.0f);
	//
	//const float4 res = float4::Shuffle<3, 0, 2, 1>(a, a) * float4::Shuffle<3, 1, 0, 2>(b, b) - float4::Shuffle<3, 0, 2, 1>(a, a) * float4::Shuffle<3, 0, 2, 1>(b, b);
	//res.CopyToAlignedData(vector);
	//
	//return Vector3(vector[3], vector[2], vector[1]);
	//

	return Vector3(_A.Y * _B.Z - _A.Z * _B.Y, _A.Z * _B.X - _A.X * _B.Z, _A.X * _B.Y - _A.Y * _B.X);
}

float BEM::DotProduct(const Quaternion& _A, const Quaternion& _B)
{
	return float4::DotProduct(float4(_A.X, _A.Y, _A.Z, _A.Q), float4(_B.X, _B.Y, _B.Z, _A.Q)).GetX();
}

float BEM::LengthSquared(const Quaternion& _A)
{
	float4 a(_A.X, _A.Y, _A.Z, _A.Q);
	return float4::DotProduct(a, a).GetX();
}

Quaternion BEM::Normalized(const Quaternion& _A)
{
	alignas(16) Quaternion result;
	auto a = float4::MakeFromUnaligned(&_A.X);
	const float4 length(Length(_A));
	a /= length;
	a.CopyToAlignedData(&result.X);
	return result;
}

void BEM::Normalize(Quaternion& _A)
{
	auto a = float4::MakeFromUnaligned(&_A.X);
	const float4 length(Length(_A));
	a /= length;
	a.CopyToUnalignedData(&_A.X);
}
