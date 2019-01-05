#include "GSMath.h"

const float PI = 3.14159265359f;

//INLINE STATIC	

//////////////////////////////////////////////////////////////
//						SCALAR MATH							//
//////////////////////////////////////////////////////////////

static float GS::Lerp(float A, float B, float Alpha)
{
	return A + Alpha * (B - A);
}

static float GS::FInterp(float Target, float Current, float DT, float InterpSpeed)
{
	return (((Target - Current) * DT) * InterpSpeed) + Current;
}

static float GS::MapToRange(float A, float InMin, float InMax, float OutMin, float OutMax)
{
	return InMin + ((OutMax - OutMin) / (InMax - InMin)) * (A - InMin);
}

static float SquareRoot(float A)
{
	//https://www.geeksforgeeks.org/square-root-of-a-perfect-square/
	float X = A;
	float Y = 1;
	float e = 0.000001f; /*e determines the level of accuracy*/
	while (X - Y > e)
	{
		X = (X + Y) / 2;
		Y = A / X;
	}
	return X;
}

static float Sine(float A)
{
	return A - (A * A * A) / 6 + (A * A * A * A * A) / 120 - (A * A * A * A * A * A * A) / 5040;
}

static float Abs(float A)
{
	return A > 0 ? A : -A;
}

int Round(float A)
{
	int Truncated = (int)A;

	if ((A - Truncated) > 0.5f)
	{
		return Truncated + 1;
	}

	else
	{
		return Truncated;
	}
}

float DegreesToRadians(float Angle)
{
	return Angle * PI / 180;
}

float RadiansToDegrees(float Radians)
{
	return Radians * 180 / PI;
}

//////////////////////////////////////////////////////////////
//						VECTOR MATH							//
//////////////////////////////////////////////////////////////

static float GS::VectorLength(const Vector2 & Vec1)
{
	return SquareRoot(Vec1.X * Vec1.X + Vec1.Y * Vec1.Y);
}

static float GS::VectorLength(const Vector3 & Vec1)
{
	return SquareRoot(Vec1.X * Vec1.X + Vec1.Y * Vec1.Y + Vec1.Z * Vec1.Z);	
}

static float GS::VectorLengthSquared(const Vector2 & Vec1)
{
	return Vec1.X * Vec1.X + Vec1.Y * Vec1.Y;
}

static float GS::VectorLengthSquared(const Vector3 & Vec1)
{
	return Vec1.X * Vec1.X + Vec1.Y * Vec1.Y + Vec1.Z * Vec1.Z;
}

static Vector2 GS::Normalize(const Vector2 & Vec1)
{
	float Length = VectorLength(Vec1);
	return { Vec1.X / Length, Vec1.Y / Length };
}

static Vector3 GS::Normalize(const Vector3 & Vec1)
{
	float Length = VectorLength(Vec1);
	return { Vec1.X / Length, Vec1.Y / Length, Vec1.Z / Length };
}

static float GS::Dot(const Vector2 & Vec1, const Vector2 & Vec2)
{
	return Vec1.X * Vec2.X + Vec1.Y * Vec2.Y;
}

static float GS::Dot(const Vector3 & Vec1, const Vector3 & Vec2)
{
	return Vec1.X * Vec2.X + Vec1.Y * Vec2.Y + Vec1.Z * Vec2.Z;
}

static Vector3 GS::Cross(const Vector3 & Vec1, const Vector3 & Vec2)
{
	return { Vec1.Y * Vec2.Z - Vec1.Z * Vec2.Y, Vec1.Z * Vec2.X - Vec1.X * Vec2.Z, Vec1.X * Vec2.Y - Vec1.Y * Vec2.X };
}

static Vector2 GS::AbsVector(const Vector2 & Vec1)
{
	return { Abs(Vec1.X), Abs(Vec1.Y) };
}

static Vector3 GS::AbsVector(const Vector3 & Vec1)
{
	return { Abs(Vec1.X), Abs(Vec1.Y), Abs(Vec1.Z) };
}


//////////////////////////////////////////////////////////////
//						ROTATOR MATH						//
//////////////////////////////////////////////////////////////







//////////////////////////////////////////////////////////////
//						LOGIC								//
//////////////////////////////////////////////////////////////

static bool GS::IsNearlyEqual(float A, float Target, float Tolerance)
{
	return A > Target - Tolerance && A < Target + Tolerance;
}

static bool GS::IsInRange(float A, float Min, float Max)
{
	return A > Min && A < Max;
}

static bool GS::IsVectorEqual(const Vector2 & A, const Vector2 & B)
{
	return A.X == B.X && A.Y == B.Y;
}

static bool GS::IsVectorEqual(const Vector3 & A, const Vector3 & B)
{
	return A.X == B.X && A.Y == B.Y && A.Z == B.Z;
}

static bool GS::IsVectorNearlyEqual(const Vector2 & A, const Vector2 & Target, float Tolerance)
{
	return IsNearlyEqual(A.X, Target.X, Tolerance) && IsNearlyEqual(A.Y, Target.Y, Tolerance);
}

static bool GS::IsVectorNearlyEqual(const Vector3 & A, const Vector3 & Target, float Tolerance)
{
	return IsNearlyEqual(A.X, Target.X, Tolerance) && IsNearlyEqual(A.Y, Target.Y, Tolerance) && IsNearlyEqual(A.Z, Target.Z, Tolerance);
}

static bool GS::AreVectorComponentsGreater(const Vector3 & A, const Vector3 & B)
{
	return A.X > B.X && A.Y > B.Y && A.Z > B.Z;
}

//////////////////////////////////////////////////////////////
//						MATRIX MATH							//
//////////////////////////////////////////////////////////////

Matrix4x4 GS::Translate(const Vector3 & Vector)
{
	Matrix4x4 Result;

	Result[0 + 3 * 4] = Vector.X;
	Result[1 + 3 * 4] = Vector.Y;
	Result[2 + 3 * 4] = Vector.Z;

	return Result;
}

/*Matrix4x4 GS::Rotate(const Quat & A)
{
	Matrix4x4 Result;
	Result.Identity();

	float r = DegreesToRadians(A.Q);
	//float cos = cos(r);
	//float sin = Sine(r);
	float omc = 1.0f - cos;

	Result.Array[0] = A.X * omc + cos;
	Result.Array[1] = A.Y * A.X * omc - A.Y * sin;
	Result.Array[2] = A.X * A.Z * omc - A.Y * sin;

	Result.Array[4] = A.X * A.Y * omc - A.Z * sin;
	Result.Array[5] = A.Y * omc + cos;
	Result.Array[6] = A.Y * A.Z * omc + A.X * sin;

	Result.Array[8] = A.X * A.Z * omc + A.Y * sin;
	Result.Array[9] = A.Y * A.Z * omc - A.X * sin;
	Result.Array[10] = A.Z * omc + cos;

	return Matrix4x4();
}
*/