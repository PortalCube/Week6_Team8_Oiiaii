#pragma once

#include "FVector.h"
#include "FMatrix.h"
#include <cmath>
#include <algorithm>

// TODO: 스타일 정리
struct FQuaternion
{
	float X = 0.0f, Y = 0.0f, Z = 0.0f, W = 1.0f; // 기본값 = 회전 없음

	FQuaternion() = default;
	FQuaternion(float InX, float InY, float InZ, float InW)
	    : X(InX), Y(InY), Z(InZ), W(InW) {}

	static FQuaternion Identity() { return FQuaternion(); }

	// 축 + 각도(도)로 생성
	// 만약 z축 기준 회전을 하고싶다면, FQuaternion qYaw = FQuaternion::FromAxisAngle(FVector(0, 0, 1), yawZ);
	static FQuaternion FromAxisAngle(const FVector& Axis, float Deg)
	{
		constexpr float DegToRad = 3.14159265358979f / 180.0f;
		const float half = Deg * DegToRad * 0.5f;
		const float s = sinf(half);

		// 축 정규화
		float len = sqrtf(Axis.X * Axis.X + Axis.Y * Axis.Y + Axis.Z * Axis.Z);
		if (len < 1e-8f)
			return Identity();
		const float inv = 1.0f / len;

		return FQuaternion(Axis.X * inv * s, Axis.Y * inv * s, Axis.Z * inv * s, cosf(half));
	}

	// 오일러각(도)에서 생성. XYZ 순서
	static FQuaternion FromEulerXYZDeg(const FVector& Deg)
	{
		return FromAxisAngle(FVector(0, 0, 1), Deg.Z) * FromAxisAngle(FVector(0, 1, 0), -Deg.Y) * FromAxisAngle(FVector(1, 0, 0), -Deg.X);
	}

	// 회전 합성. 교환법칙 성립 안 함
	FQuaternion operator*(const FQuaternion& Q) const
	{
		return FQuaternion(
		    W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y,
		    W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X,
		    W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
		    W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z);
	}

	FQuaternion& operator*=(const FQuaternion& Q)
	{
		*this = *this * Q;
		return *this;
	}

	// 켤레 = 역회전 (단위 쿼터니언 기준)
	FQuaternion Conjugate() const { return FQuaternion(-X, -Y, -Z, W); }

	float LengthSquared() const { return X * X + Y * Y + Z * Z + W * W; }

	float Length() const { return sqrtf(LengthSquared()); }

	void Normalize()
	{
		float lenSq = LengthSquared();
		if (lenSq < 1e-8f)
		{
			*this = Identity();
			return;
		}
		const float inv = 1.0f / sqrtf(lenSq);
		X *= inv;
		Y *= inv;
		Z *= inv;
		W *= inv;
	}

	FQuaternion Normalized() const
	{
		float l = Length();
		if (l <= 0.0f)
			return Identity();
		float inv = 1.0f / l;
		return FQuaternion(X * inv, Y * inv, Z * inv, W * inv);
	}

	FMatrix ToMatrixRow() const
	{
		FQuaternion q = Normalized();

		float x = q.X, y = q.Y, z = q.Z, w = q.W;
		float xx = x * x, yy = y * y, zz = z * z;
		float xy = x * y, xz = x * z, yz = y * z;
		float wx = w * x, wy = w * y, wz = w * z;

		FMatrix R = FMatrix::Identity;

		R.M[0][0] = 1.0f - 2.0f * (yy + zz);
		R.M[1][0] = 2.0f * (xy - wz);
		R.M[2][0] = 2.0f * (xz + wy);

		R.M[0][1] = 2.0f * (xy + wz);
		R.M[1][1] = 1.0f - 2.0f * (xx + zz);
		R.M[2][1] = 2.0f * (yz - wx);

		R.M[0][2] = 2.0f * (xz - wy);
		R.M[1][2] = 2.0f * (yz + wx);
		R.M[2][2] = 1.0f - 2.0f * (xx + yy);
		return R;
	}

	// 벡터 회전: v' = q * v * q⁻¹ 를 전개한 형태
	FVector RotateVector(const FVector& V) const
	{
		const FVector u(X, Y, Z);
		const float uv = u.X * V.X + u.Y * V.Y + u.Z * V.Z;
		const float uu = u.X * u.X + u.Y * u.Y + u.Z * u.Z;

		// cross(u, v)
		const FVector cross(
		    u.Y * V.Z - u.Z * V.Y,
		    u.Z * V.X - u.X * V.Z,
		    u.X * V.Y - u.Y * V.X);

		return FVector(
		    2.0f * uv * u.X + (W * W - uu) * V.X + 2.0f * W * cross.X,
		    2.0f * uv * u.Y + (W * W - uu) * V.Y + 2.0f * W * cross.Y,
		    2.0f * uv * u.Z + (W * W - uu) * V.Z + 2.0f * W * cross.Z);
	}

	// 월드 축 기준 회전 추가
	void RotateWorldAxisAngle(const FVector& Axis, float Deg)
	{
		*this = FromAxisAngle(Axis, Deg) * (*this);
		Normalize();
	}

	// 로컬 축 기준 회전 추가
	void RotateLocalAxisAngle(const FVector& Axis, float Deg)
	{
		*this = (*this) * FromAxisAngle(Axis, Deg);
		Normalize();
	}

	// FromEulerXYZDeg의 역변환. 반환 단위는 라디안입니다.
	FVector GetEulerXYZ() const
	{
		// 특이점 부근의 작은 행렬 성분이 float 연산으로 소실되지 않도록
		// 정규화와 필요한 행렬 성분 계산을 double로 수행합니다.
		const double x = X, y = Y, z = Z, w = W;
		const double NormSquared = x * x + y * y + z * z + w * w;
		if (NormSquared == 0.0)
			return FVector(0.0f, 0.0f, 0.0f);
		const double S = 2.0 / NormSquared;
		const double R00 = 1.0 - S * (y * y + z * z);
		const double R01 = S * (x * y + w * z);
		const double R10 = S * (x * y - w * z);
		const double R11 = 1.0 - S * (x * x + z * z);
		const double R12 = S * (y * z + w * x);
		const double R22 = 1.0 - S * (x * x + y * y);
		const double sy = std::clamp(S * (x * z - w * y), -1.0, 1.0);
		double rx, rz;
		// asin(sy)는 ±90도 부근에서 sy가 ±1로 반올림되면 정보를 잃습니다.
		const double cy = std::hypot(R00, R01);
		const double ry = std::atan2(sy, cy);

		// 짐벌락 부근에서는 불안정한 X/Z 분리 대신 Z를 0으로 고정합니다.
		if (cy > 1e-6)
		{
			rx = std::atan2(-R12, R22);
			rz = std::atan2(R01, R00);
		}
		else
		{
			rz = 0.0f;
			rx = std::atan2((sy > 0.0 ? 1.0 : -1.0) * R10, R11);
		}
		return FVector(static_cast<float>(rx), static_cast<float>(ry), static_cast<float>(rz));
	}

	// 디그리 단위 오일러 각도 반환
	FVector ToEulerXYZDeg() const
	{
		constexpr float RadToDeg = 180.0f / 3.14159265358979f;
		FVector Rad = GetEulerXYZ();
		return FVector(Rad.X * RadToDeg, Rad.Y * RadToDeg, Rad.Z * RadToDeg);
	}
};
