#include <cstdlib>
#include <random>

#include "ue5parse/runtime/Core/Math/UnrealMath.h"

// The math functions that are not inline (UE's UnrealMath.cpp, Color.cpp).

namespace ue5parse::runtime {

FVector2D FMath::GetAzimuthAndElevation(const FVector& Direction, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ) {
    const FVector NormalDir = Direction.GetSafeNormal();
    // The direction projected onto the plane of AxisX and AxisY.
    const FVector NoZProjDir = (NormalDir - (NormalDir | AxisZ) * AxisZ).GetSafeNormal();
    const double AzimuthSign = (NoZProjDir | AxisY) < 0.0 ? -1.0 : 1.0;
    const double ElevationSin = NormalDir | AxisZ;
    const double AzimuthCos = NoZProjDir | AxisX;
    return FVector2D(Acos(AzimuthCos) * AzimuthSign, Asin(ElevationSin));
}

FVector FMath::GetBaryCentric2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C) {
    const double Denominator = (B.Y - C.Y) * (A.X - C.X) + (C.X - B.X) * (A.Y - C.Y);
    const double a = ((B.Y - C.Y) * (Point.X - C.X) + (C.X - B.X) * (Point.Y - C.Y)) / Denominator;
    const double b = ((C.Y - A.Y) * (Point.X - C.X) + (A.X - C.X) * (Point.Y - C.Y)) / Denominator;
    return FVector(a, b, 1.0 - a - b);
}

FVector FMath::ClosestPointOnSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint) {
    const FVector Segment = EndPoint - StartPoint;
    const FVector VectToPoint = Point - StartPoint;
    const double Dot1 = VectToPoint | Segment;
    if (Dot1 <= 0) return StartPoint;
    const double Dot2 = Segment | Segment;
    if (Dot2 <= Dot1) return EndPoint;
    return StartPoint + Segment * (Dot1 / Dot2);
}

double FMath::Fmod(double X, double Y) {
    const double AbsY = Abs(Y);
    if (AbsY <= UE_DOUBLE_SMALL_NUMBER) return 0.0;
    return std::fmod(X, Y);
}

float FMath::Fmod(float X, float Y) {
    const float AbsY = Abs(Y);
    if (AbsY <= UE_SMALL_NUMBER) return 0.0f;
    return std::fmod(X, Y);
}

double FMath::FastAsin(double Value) {
    constexpr double FastAsinHalfPi = 1.5707963050;
    const bool bNonNegative = Value >= 0.0;
    const double X = Abs(Value);
    double OneMinusX = 1.0 - X;
    if (OneMinusX < 0.0) OneMinusX = 0.0;
    const double Root = std::sqrt(OneMinusX);
    double Result = ((((((-0.0012624911 * X + 0.0066700901) * X - 0.0170881256) * X + 0.0308918810) * X - 0.0501743046) * X + 0.0889789874) * X -
                     0.2145988016) * X + FastAsinHalfPi;
    Result *= Root;
    return bNonNegative ? FastAsinHalfPi - Result : Result - FastAsinHalfPi;
}

double FMath::ClampAngle(double AngleDegrees, double MinAngleDegrees, double MaxAngleDegrees) {
    const double MaxDelta = FRotator::ClampAxis(MaxAngleDegrees - MinAngleDegrees) * 0.5;
    const double RangeCenter = FRotator::ClampAxis(MinAngleDegrees + MaxDelta);
    const double DeltaFromCenter = FRotator::NormalizeAxis(AngleDegrees - RangeCenter);
    if (DeltaFromCenter > MaxDelta) return FRotator::NormalizeAxis(RangeCenter + MaxDelta);
    if (DeltaFromCenter < -MaxDelta) return FRotator::NormalizeAxis(RangeCenter - MaxDelta);
    return FRotator::NormalizeAxis(AngleDegrees);
}

namespace {

std::mt19937& GetRandomEngine() {
    static thread_local std::mt19937 Engine(0x5eed1234u);
    return Engine;
}

}

int32 FMath::Rand() { return static_cast<int32>(GetRandomEngine()() & 0x7fffffff); }

void FMath::RandInit(int32 Seed) { GetRandomEngine().seed(static_cast<uint32>(Seed)); }

double FMath::FRand() {
    // [0, 1), as UE's Rand() / (RAND_MAX + 1).
    return static_cast<double>(Rand()) / 2147483648.0;
}

// --- Rotator ---

template <class T>
TVector<T> TRotator<T>::Vector() const {
    T CP, SP, CY, SY;
    const T PitchNoWinding = static_cast<T>(FMath::Fmod(static_cast<double>(Pitch), 360.0));
    const T YawNoWinding = static_cast<T>(FMath::Fmod(static_cast<double>(Yaw), 360.0));
    FMath::SinCos(&SP, &CP, FMath::DegreesToRadians(PitchNoWinding));
    FMath::SinCos(&SY, &CY, FMath::DegreesToRadians(YawNoWinding));
    return TVector<T>(CP * CY, CP * SY, SP);
}

template <class T>
TQuat<T> TRotator<T>::Quaternion() const {
    const T RadsDividedBy2 = static_cast<T>(UE_PI / 180.0) / T(2);
    T SP, SY, SR, CP, CY, CR;
    const T PitchNoWinding = static_cast<T>(FMath::Fmod(static_cast<double>(Pitch), 360.0));
    const T YawNoWinding = static_cast<T>(FMath::Fmod(static_cast<double>(Yaw), 360.0));
    const T RollNoWinding = static_cast<T>(FMath::Fmod(static_cast<double>(Roll), 360.0));
    FMath::SinCos(&SP, &CP, PitchNoWinding * RadsDividedBy2);
    FMath::SinCos(&SY, &CY, YawNoWinding * RadsDividedBy2);
    FMath::SinCos(&SR, &CR, RollNoWinding * RadsDividedBy2);
    TQuat<T> RotationQuat;
    RotationQuat.X = CR * SP * SY - SR * CP * CY;
    RotationQuat.Y = -CR * SP * CY - SR * CP * SY;
    RotationQuat.Z = CR * CP * SY - SR * SP * CY;
    RotationQuat.W = CR * CP * CY + SR * SP * SY;
    return RotationQuat;
}

template <class T>
TRotator<T> TRotator<T>::GridSnap(const TRotator& RotGrid) const {
    auto Snap = [](T Location, T Grid) { return Grid == T(0) ? Location : std::floor((Location + Grid / T(2)) / Grid) * Grid; };
    return TRotator(Snap(Pitch, RotGrid.Pitch), Snap(Yaw, RotGrid.Yaw), Snap(Roll, RotGrid.Roll));
}

template <class T>
void TRotator<T>::SetClosestToMe(TRotator& MakeClosest) const {
    TRotator OtherChoice = MakeClosest.GetEquivalentRotator();
    const T FirstDiff = FMath::Abs(NormalizeAxis(Pitch - MakeClosest.Pitch)) + FMath::Abs(NormalizeAxis(Yaw - MakeClosest.Yaw)) +
                        FMath::Abs(NormalizeAxis(Roll - MakeClosest.Roll));
    const T SecondDiff = FMath::Abs(NormalizeAxis(Pitch - OtherChoice.Pitch)) + FMath::Abs(NormalizeAxis(Yaw - OtherChoice.Yaw)) +
                         FMath::Abs(NormalizeAxis(Roll - OtherChoice.Roll));
    if (SecondDiff < FirstDiff) MakeClosest = OtherChoice;
}

template <class T>
TRotator<T> TVector<T>::ToOrientationRotator() const {
    TRotator<T> R;
    R.Yaw = FMath::RadiansToDegrees(std::atan2(Y, X));
    R.Pitch = FMath::RadiansToDegrees(std::atan2(Z, std::sqrt(X * X + Y * Y)));
    R.Roll = 0;
    return R;
}

template <class T>
TRotator<T> TVector<T>::Rotation() const {
    return ToOrientationRotator();
}

// --- Quat ---

template <class T>
TRotator<T> TQuat<T>::Rotator() const {
    const T SingularityTest = Z * X - W * Y;
    const T YawY = T(2) * (W * Z + X * Y);
    const T YawX = T(1) - T(2) * (FMath::Square(Y) + FMath::Square(Z));
    const T SingularityThreshold = T(0.4999995);
    const T RadToDeg = static_cast<T>(180.0 / UE_PI);
    TRotator<T> Result;
    if (SingularityTest < -SingularityThreshold) {
        Result.Pitch = T(-90);
        Result.Yaw = std::atan2(YawY, YawX) * RadToDeg;
        Result.Roll = TRotator<T>::NormalizeAxis(-Result.Yaw - (T(2) * std::atan2(X, W) * RadToDeg));
    } else if (SingularityTest > SingularityThreshold) {
        Result.Pitch = T(90);
        Result.Yaw = std::atan2(YawY, YawX) * RadToDeg;
        Result.Roll = TRotator<T>::NormalizeAxis(Result.Yaw - (T(2) * std::atan2(X, W) * RadToDeg));
    } else {
        Result.Pitch = static_cast<T>(FMath::FastAsin(static_cast<double>(T(2) * SingularityTest))) * RadToDeg;
        Result.Yaw = std::atan2(YawY, YawX) * RadToDeg;
        Result.Roll = std::atan2(T(-2) * (W * X + Y * Z), T(1) - T(2) * (FMath::Square(X) + FMath::Square(Y))) * RadToDeg;
    }
    return Result;
}

template <class T>
TQuat<T> TQuat<T>::Slerp_NotNormalized(const TQuat& Quat1, const TQuat& Quat2, T Slerp) {
    const T RawCosom = Quat1 | Quat2;
    const T Cosom = RawCosom >= T(0) ? RawCosom : -RawCosom;
    T Scale0, Scale1;
    if (Cosom < T(0.9999f)) {
        const T Omega = std::acos(Cosom);
        const T InvSin = T(1) / std::sin(Omega);
        Scale0 = std::sin((T(1) - Slerp) * Omega) * InvSin;
        Scale1 = std::sin(Slerp * Omega) * InvSin;
    } else {
        Scale0 = T(1) - Slerp;
        Scale1 = Slerp;
    }
    Scale1 = RawCosom >= T(0) ? Scale1 : -Scale1;
    return TQuat(Scale0 * Quat1.X + Scale1 * Quat2.X, Scale0 * Quat1.Y + Scale1 * Quat2.Y, Scale0 * Quat1.Z + Scale1 * Quat2.Z,
                 Scale0 * Quat1.W + Scale1 * Quat2.W);
}

template <class T>
TQuat<T> TQuat<T>::SlerpFullPath_NotNormalized(const TQuat& Quat1, const TQuat& Quat2, T Alpha) {
    const T CosAngle = FMath::Clamp(Quat1 | Quat2, T(-1), T(1));
    const T Angle = std::acos(CosAngle);
    if (FMath::Abs(Angle) < T(UE_KINDA_SMALL_NUMBER)) return Quat1;
    const T SinAngle = std::sin(Angle);
    const T InvSinAngle = T(1) / SinAngle;
    const T Scale0 = std::sin((T(1) - Alpha) * Angle) * InvSinAngle;
    const T Scale1 = std::sin(Alpha * Angle) * InvSinAngle;
    return Quat1 * Scale0 + Quat2 * Scale1;
}

template <class T>
TQuat<T> TQuat<T>::FindBetweenHelper(const TVector<T>& A, const TVector<T>& B, T NormAB) {
    T NewW = NormAB + TVector<T>::DotProduct(A, B);
    TQuat Result;
    if (NewW >= T(1e-6f) * NormAB) {
        Result = TQuat(A.Y * B.Z - A.Z * B.Y, A.Z * B.X - A.X * B.Z, A.X * B.Y - A.Y * B.X, NewW);
    } else {
        NewW = T(0);
        Result = FMath::Abs(A.X) > FMath::Abs(A.Y) ? TQuat(-A.Z, T(0), A.X, NewW) : TQuat(T(0), -A.Z, A.Y, NewW);
    }
    Result.Normalize();
    return Result;
}

template <class T>
TQuat<T> TQuat<T>::Log() const {
    TQuat Result;
    Result.W = T(0);
    if (FMath::Abs(W) < T(1)) {
        const T Angle = std::acos(W);
        const T SinAngle = std::sin(Angle);
        if (FMath::Abs(SinAngle) >= T(UE_SMALL_NUMBER)) {
            const T Scale = Angle / SinAngle;
            Result.X = Scale * X;
            Result.Y = Scale * Y;
            Result.Z = Scale * Z;
            return Result;
        }
    }
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    return Result;
}

template <class T>
TQuat<T> TQuat<T>::Exp() const {
    const T Angle = std::sqrt(X * X + Y * Y + Z * Z);
    const T SinAngle = std::sin(Angle);
    TQuat Result;
    Result.W = std::cos(Angle);
    if (FMath::Abs(SinAngle) >= T(UE_SMALL_NUMBER)) {
        const T Scale = SinAngle / Angle;
        Result.X = Scale * X;
        Result.Y = Scale * Y;
        Result.Z = Scale * Z;
    } else {
        Result.X = X;
        Result.Y = Y;
        Result.Z = Z;
    }
    return Result;
}

// --- Matrix ---

template <class T>
T TMatrix<T>::Determinant() const {
    return M[0][0] * (M[1][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) - M[2][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) +
                      M[3][1] * (M[1][2] * M[2][3] - M[1][3] * M[2][2])) -
           M[1][0] * (M[0][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) - M[2][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) +
                      M[3][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2])) +
           M[2][0] * (M[0][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) - M[1][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) +
                      M[3][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2])) -
           M[3][0] * (M[0][1] * (M[1][2] * M[2][3] - M[1][3] * M[2][2]) - M[1][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2]) +
                      M[2][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2]));
}

template <class T>
TMatrix<T> TMatrix<T>::InverseFast() const {
    // The adjugate divided by the determinant.
    TMatrix Result;
    T Tmp[4][4];
    const T* A = &M[0][0];
    T* Inv = &Tmp[0][0];
    Inv[0] = A[5] * A[10] * A[15] - A[5] * A[11] * A[14] - A[9] * A[6] * A[15] + A[9] * A[7] * A[14] + A[13] * A[6] * A[11] - A[13] * A[7] * A[10];
    Inv[4] = -A[4] * A[10] * A[15] + A[4] * A[11] * A[14] + A[8] * A[6] * A[15] - A[8] * A[7] * A[14] - A[12] * A[6] * A[11] + A[12] * A[7] * A[10];
    Inv[8] = A[4] * A[9] * A[15] - A[4] * A[11] * A[13] - A[8] * A[5] * A[15] + A[8] * A[7] * A[13] + A[12] * A[5] * A[11] - A[12] * A[7] * A[9];
    Inv[12] = -A[4] * A[9] * A[14] + A[4] * A[10] * A[13] + A[8] * A[5] * A[14] - A[8] * A[6] * A[13] - A[12] * A[5] * A[10] + A[12] * A[6] * A[9];
    Inv[1] = -A[1] * A[10] * A[15] + A[1] * A[11] * A[14] + A[9] * A[2] * A[15] - A[9] * A[3] * A[14] - A[13] * A[2] * A[11] + A[13] * A[3] * A[10];
    Inv[5] = A[0] * A[10] * A[15] - A[0] * A[11] * A[14] - A[8] * A[2] * A[15] + A[8] * A[3] * A[14] + A[12] * A[2] * A[11] - A[12] * A[3] * A[10];
    Inv[9] = -A[0] * A[9] * A[15] + A[0] * A[11] * A[13] + A[8] * A[1] * A[15] - A[8] * A[3] * A[13] - A[12] * A[1] * A[11] + A[12] * A[3] * A[9];
    Inv[13] = A[0] * A[9] * A[14] - A[0] * A[10] * A[13] - A[8] * A[1] * A[14] + A[8] * A[2] * A[13] + A[12] * A[1] * A[10] - A[12] * A[2] * A[9];
    Inv[2] = A[1] * A[6] * A[15] - A[1] * A[7] * A[14] - A[5] * A[2] * A[15] + A[5] * A[3] * A[14] + A[13] * A[2] * A[7] - A[13] * A[3] * A[6];
    Inv[6] = -A[0] * A[6] * A[15] + A[0] * A[7] * A[14] + A[4] * A[2] * A[15] - A[4] * A[3] * A[14] - A[12] * A[2] * A[7] + A[12] * A[3] * A[6];
    Inv[10] = A[0] * A[5] * A[15] - A[0] * A[7] * A[13] - A[4] * A[1] * A[15] + A[4] * A[3] * A[13] + A[12] * A[1] * A[7] - A[12] * A[3] * A[5];
    Inv[14] = -A[0] * A[5] * A[14] + A[0] * A[6] * A[13] + A[4] * A[1] * A[14] - A[4] * A[2] * A[13] - A[12] * A[1] * A[6] + A[12] * A[2] * A[5];
    Inv[3] = -A[1] * A[6] * A[11] + A[1] * A[7] * A[10] + A[5] * A[2] * A[11] - A[5] * A[3] * A[10] - A[9] * A[2] * A[7] + A[9] * A[3] * A[6];
    Inv[7] = A[0] * A[6] * A[11] - A[0] * A[7] * A[10] - A[4] * A[2] * A[11] + A[4] * A[3] * A[10] + A[8] * A[2] * A[7] - A[8] * A[3] * A[6];
    Inv[11] = -A[0] * A[5] * A[11] + A[0] * A[7] * A[9] + A[4] * A[1] * A[11] - A[4] * A[3] * A[9] - A[8] * A[1] * A[7] + A[8] * A[3] * A[5];
    Inv[15] = A[0] * A[5] * A[10] - A[0] * A[6] * A[9] - A[4] * A[1] * A[10] + A[4] * A[2] * A[9] + A[8] * A[1] * A[6] - A[8] * A[2] * A[5];
    T Det = A[0] * Inv[0] + A[1] * Inv[4] + A[2] * Inv[8] + A[3] * Inv[12];
    if (Det == T(0)) return TMatrix();
    const T InvDet = T(1) / Det;
    for (int32 i = 0; i < 4; i++)
        for (int32 j = 0; j < 4; j++) Result.M[i][j] = Tmp[i][j] * InvDet;
    return Result;
}

template <class T>
TMatrix<T> TMatrix<T>::Inverse() const {
    // A matrix with a zero scale axis has no inverse: UE returns the identity.
    for (int32 i = 0; i < 3; i++)
        if (GetScaledAxis(i).IsNearlyZero(T(UE_SMALL_NUMBER))) return TMatrix();
    if (Determinant() == T(0)) return TMatrix();
    return InverseFast();
}

template <class T>
TMatrix<T> TMatrix<T>::TransposeAdjoint() const {
    TMatrix TA;
    TA.M[0][0] = M[1][1] * M[2][2] - M[1][2] * M[2][1];
    TA.M[0][1] = M[1][2] * M[2][0] - M[1][0] * M[2][2];
    TA.M[0][2] = M[1][0] * M[2][1] - M[1][1] * M[2][0];
    TA.M[0][3] = 0;
    TA.M[1][0] = M[2][1] * M[0][2] - M[2][2] * M[0][1];
    TA.M[1][1] = M[2][2] * M[0][0] - M[2][0] * M[0][2];
    TA.M[1][2] = M[2][0] * M[0][1] - M[2][1] * M[0][0];
    TA.M[1][3] = 0;
    TA.M[2][0] = M[0][1] * M[1][2] - M[0][2] * M[1][1];
    TA.M[2][1] = M[0][2] * M[1][0] - M[0][0] * M[1][2];
    TA.M[2][2] = M[0][0] * M[1][1] - M[0][1] * M[1][0];
    TA.M[2][3] = 0;
    TA.M[3][0] = 0;
    TA.M[3][1] = 0;
    TA.M[3][2] = 0;
    TA.M[3][3] = 1;
    return TA;
}

template <class T>
void TMatrix<T>::RemoveScaling(T Tolerance) {
    for (int32 i = 0; i < 3; i++) {
        const T SquareSum = M[i][0] * M[i][0] + M[i][1] * M[i][1] + M[i][2] * M[i][2];
        const T Scale = SquareSum - Tolerance >= T(0) ? T(1) / std::sqrt(SquareSum) : T(1);
        M[i][0] *= Scale;
        M[i][1] *= Scale;
        M[i][2] *= Scale;
    }
}

template <class T>
TVector<T> TMatrix<T>::ExtractScaling(T Tolerance) {
    TVector<T> Scale3D(0, 0, 0);
    for (int32 i = 0; i < 3; i++) {
        const T SquareSum = M[i][0] * M[i][0] + M[i][1] * M[i][1] + M[i][2] * M[i][2];
        if (SquareSum > Tolerance) {
            const T Scale = std::sqrt(SquareSum);
            Scale3D[i] = Scale;
            const T InvScale = T(1) / Scale;
            M[i][0] *= InvScale;
            M[i][1] *= InvScale;
            M[i][2] *= InvScale;
        } else {
            Scale3D[i] = T(0);
        }
    }
    return Scale3D;
}

template <class T>
TVector<T> TMatrix<T>::GetScaleVector(T Tolerance) const {
    TVector<T> Scale3D(1, 1, 1);
    for (int32 i = 0; i < 3; i++) {
        const T SquareSum = M[i][0] * M[i][0] + M[i][1] * M[i][1] + M[i][2] * M[i][2];
        Scale3D[i] = SquareSum > Tolerance ? std::sqrt(SquareSum) : T(0);
    }
    return Scale3D;
}

template <class T>
TRotator<T> TMatrix<T>::Rotator() const {
    const TVector<T> XAxis = GetScaledAxis(0);
    const TVector<T> YAxis = GetScaledAxis(1);
    const TVector<T> ZAxis = GetScaledAxis(2);
    const T RadToDeg = static_cast<T>(180.0 / UE_PI);
    TRotator<T> Result(std::atan2(XAxis.Z, std::sqrt(FMath::Square(XAxis.X) + FMath::Square(XAxis.Y))) * RadToDeg,
                       std::atan2(XAxis.Y, XAxis.X) * RadToDeg, T(0));
    const TVector<T> SYAxis = MakeRotationTranslationMatrix(Result, TVector<T>()).GetScaledAxis(1);
    Result.Roll = std::atan2(ZAxis | SYAxis, YAxis | SYAxis) * RadToDeg;
    return Result;
}

template <class T>
TQuat<T> TMatrix<T>::ToQuat() const {
    if (GetScaledAxis(0).IsNearlyZero() || GetScaledAxis(1).IsNearlyZero() || GetScaledAxis(2).IsNearlyZero()) return TQuat<T>::Identity;
    TQuat<T> Q;
    const T Trace = M[0][0] + M[1][1] + M[2][2];
    if (Trace > T(0)) {
        const T InvS = T(1) / std::sqrt(Trace + T(1));
        Q.W = T(0.5) * (T(1) / InvS);
        const T S = T(0.5) * InvS;
        Q.X = (M[1][2] - M[2][1]) * S;
        Q.Y = (M[2][0] - M[0][2]) * S;
        Q.Z = (M[0][1] - M[1][0]) * S;
    } else {
        int32 i = 0;
        if (M[1][1] > M[0][0]) i = 1;
        if (M[2][2] > M[i][i]) i = 2;
        static constexpr int32 Next[3] = {1, 2, 0};
        const int32 j = Next[i];
        const int32 k = Next[j];
        T S = M[i][i] - M[j][j] - M[k][k] + T(1);
        const T InvS = T(1) / std::sqrt(S);
        T Qt[4];
        Qt[i] = T(0.5) * (T(1) / InvS);
        S = T(0.5) * InvS;
        Qt[3] = (M[j][k] - M[k][j]) * S;
        Qt[j] = (M[i][j] + M[j][i]) * S;
        Qt[k] = (M[i][k] + M[k][i]) * S;
        Q.X = Qt[0];
        Q.Y = Qt[1];
        Q.Z = Qt[2];
        Q.W = Qt[3];
    }
    return Q;
}

template <class T>
TMatrix<T> MakeRotationTranslationMatrix(const TRotator<T>& Rot, const TVector<T>& Origin) {
    T SP, SY, SR, CP, CY, CR;
    FMath::SinCos(&SP, &CP, FMath::DegreesToRadians(Rot.Pitch));
    FMath::SinCos(&SY, &CY, FMath::DegreesToRadians(Rot.Yaw));
    FMath::SinCos(&SR, &CR, FMath::DegreesToRadians(Rot.Roll));
    TMatrix<T> R;
    R.M[0][0] = CP * CY;
    R.M[0][1] = CP * SY;
    R.M[0][2] = SP;
    R.M[0][3] = 0;
    R.M[1][0] = SR * SP * CY - CR * SY;
    R.M[1][1] = SR * SP * SY + CR * CY;
    R.M[1][2] = -SR * CP;
    R.M[1][3] = 0;
    R.M[2][0] = -(CR * SP * CY + SR * SY);
    R.M[2][1] = CY * SR - CR * SP * SY;
    R.M[2][2] = CR * CP;
    R.M[2][3] = 0;
    R.M[3][0] = Origin.X;
    R.M[3][1] = Origin.Y;
    R.M[3][2] = Origin.Z;
    R.M[3][3] = 1;
    return R;
}

template <class T>
TMatrix<T> MakeQuatRotationTranslationMatrix(const TQuat<T>& Q, const TVector<T>& Origin) {
    const T x2 = Q.X + Q.X, y2 = Q.Y + Q.Y, z2 = Q.Z + Q.Z;
    const T xx = Q.X * x2, xy = Q.X * y2, xz = Q.X * z2;
    const T yy = Q.Y * y2, yz = Q.Y * z2, zz = Q.Z * z2;
    const T wx = Q.W * x2, wy = Q.W * y2, wz = Q.W * z2;
    TMatrix<T> R;
    R.M[0][0] = T(1) - (yy + zz);
    R.M[1][0] = xy - wz;
    R.M[2][0] = xz + wy;
    R.M[3][0] = Origin.X;
    R.M[0][1] = xy + wz;
    R.M[1][1] = T(1) - (xx + zz);
    R.M[2][1] = yz - wx;
    R.M[3][1] = Origin.Y;
    R.M[0][2] = xz - wy;
    R.M[1][2] = yz + wx;
    R.M[2][2] = T(1) - (xx + yy);
    R.M[3][2] = Origin.Z;
    R.M[0][3] = 0;
    R.M[1][3] = 0;
    R.M[2][3] = 0;
    R.M[3][3] = 1;
    return R;
}

namespace {

constexpr double AxisTolerance = 1.0 - UE_KINDA_SMALL_NUMBER;

bool AreParallel(const FVector& A, const FVector& B) { return FMath::IsNearlyEqual(FMath::Abs(A | B), 1.0); }

}

FMatrix FRotationMatrix::MakeFromX(const FVector& XAxis) {
    const FVector NewX = XAxis.GetSafeNormal();
    const FVector UpVector = FMath::Abs(NewX.Z) < AxisTolerance ? FVector(0, 0, 1) : FVector(1, 0, 0);
    const FVector NewY = (UpVector ^ NewX).GetSafeNormal();
    const FVector NewZ = NewX ^ NewY;
    return FMatrix(NewX, NewY, NewZ, FVector::ZeroVector);
}

FMatrix FRotationMatrix::MakeFromY(const FVector& YAxis) {
    const FVector NewY = YAxis.GetSafeNormal();
    const FVector UpVector = FMath::Abs(NewY.Z) < AxisTolerance ? FVector(0, 0, 1) : FVector(1, 0, 0);
    const FVector NewZ = (UpVector ^ NewY).GetSafeNormal();
    const FVector NewX = NewY ^ NewZ;
    return FMatrix(NewX, NewY, NewZ, FVector::ZeroVector);
}

FMatrix FRotationMatrix::MakeFromZ(const FVector& ZAxis) {
    const FVector NewZ = ZAxis.GetSafeNormal();
    const FVector UpVector = FMath::Abs(NewZ.Z) < AxisTolerance ? FVector(1, 0, 0) : FVector(0, 1, 0);
    const FVector NewY = (NewZ ^ UpVector).GetSafeNormal();
    const FVector NewX = NewY ^ NewZ;
    return FMatrix(NewX, NewY, NewZ, FVector::ZeroVector);
}

FMatrix FRotationMatrix::MakeFromXY(const FVector& XAxis, const FVector& YAxis) {
    FVector NewX = XAxis.GetSafeNormal();
    FVector Norm = YAxis.GetSafeNormal();
    if (AreParallel(NewX, Norm)) Norm = FMath::Abs(NewX.Z) < AxisTolerance ? FVector(0, 0, 1) : FVector(1, 0, 0);
    const FVector NewZ = (NewX ^ Norm).GetSafeNormal();
    const FVector NewY = NewZ ^ NewX;
    return FMatrix(NewX, NewY, NewZ, FVector::ZeroVector);
}

FMatrix FRotationMatrix::MakeFromXZ(const FVector& XAxis, const FVector& ZAxis) {
    const FVector NewX = XAxis.GetSafeNormal();
    FVector Norm = ZAxis.GetSafeNormal();
    if (AreParallel(NewX, Norm)) Norm = FMath::Abs(NewX.Z) < AxisTolerance ? FVector(0, 0, 1) : FVector(1, 0, 0);
    const FVector NewY = (Norm ^ NewX).GetSafeNormal();
    const FVector NewZ = NewX ^ NewY;
    return FMatrix(NewX, NewY, NewZ, FVector::ZeroVector);
}

FMatrix FRotationMatrix::MakeFromYX(const FVector& YAxis, const FVector& XAxis) {
    const FVector NewY = YAxis.GetSafeNormal();
    FVector Norm = XAxis.GetSafeNormal();
    if (AreParallel(NewY, Norm)) Norm = FMath::Abs(NewY.Z) < AxisTolerance ? FVector(0, 0, 1) : FVector(1, 0, 0);
    const FVector NewZ = (Norm ^ NewY).GetSafeNormal();
    const FVector NewX = NewY ^ NewZ;
    return FMatrix(NewX, NewY, NewZ, FVector::ZeroVector);
}

FMatrix FRotationMatrix::MakeFromYZ(const FVector& YAxis, const FVector& ZAxis) {
    const FVector NewY = YAxis.GetSafeNormal();
    FVector Norm = ZAxis.GetSafeNormal();
    if (AreParallel(NewY, Norm)) Norm = FMath::Abs(NewY.Z) < AxisTolerance ? FVector(0, 0, 1) : FVector(1, 0, 0);
    const FVector NewX = (NewY ^ Norm).GetSafeNormal();
    const FVector NewZ = NewX ^ NewY;
    return FMatrix(NewX, NewY, NewZ, FVector::ZeroVector);
}

FMatrix FRotationMatrix::MakeFromZX(const FVector& ZAxis, const FVector& XAxis) {
    const FVector NewZ = ZAxis.GetSafeNormal();
    FVector Norm = XAxis.GetSafeNormal();
    if (AreParallel(NewZ, Norm)) Norm = FMath::Abs(NewZ.Z) < AxisTolerance ? FVector(1, 0, 0) : FVector(0, 1, 0);
    const FVector NewY = (NewZ ^ Norm).GetSafeNormal();
    const FVector NewX = NewY ^ NewZ;
    return FMatrix(NewX, NewY, NewZ, FVector::ZeroVector);
}

FMatrix FRotationMatrix::MakeFromZY(const FVector& ZAxis, const FVector& YAxis) {
    const FVector NewZ = ZAxis.GetSafeNormal();
    FVector Norm = YAxis.GetSafeNormal();
    if (AreParallel(NewZ, Norm)) Norm = FMath::Abs(NewZ.Z) < AxisTolerance ? FVector(1, 0, 0) : FVector(0, 1, 0);
    const FVector NewX = (Norm ^ NewZ).GetSafeNormal();
    const FVector NewY = NewZ ^ NewX;
    return FMatrix(NewX, NewY, NewZ, FVector::ZeroVector);
}

FLookAtMatrix::FLookAtMatrix(const FVector& EyePosition, const FVector& LookAtPosition, const FVector& UpVector) {
    const FVector ZAxis = (LookAtPosition - EyePosition).GetSafeNormal();
    const FVector XAxis = (UpVector ^ ZAxis).GetSafeNormal();
    const FVector YAxis = ZAxis ^ XAxis;
    for (int32 RowIndex = 0; RowIndex < 3; RowIndex++) {
        M[RowIndex][0] = (&XAxis.X)[RowIndex];
        M[RowIndex][1] = (&YAxis.X)[RowIndex];
        M[RowIndex][2] = (&ZAxis.X)[RowIndex];
        M[RowIndex][3] = 0;
    }
    M[3][0] = -EyePosition | XAxis;
    M[3][1] = -EyePosition | YAxis;
    M[3][2] = -EyePosition | ZAxis;
    M[3][3] = 1;
}

// --- Color ---

const FLinearColor FLinearColor::White(1, 1, 1);
const FLinearColor FLinearColor::Gray(0.5f, 0.5f, 0.5f);
const FLinearColor FLinearColor::Black(0, 0, 0);
const FLinearColor FLinearColor::Transparent(0, 0, 0, 0);
const FLinearColor FLinearColor::Red(1, 0, 0);
const FLinearColor FLinearColor::Green(0, 1, 0);
const FLinearColor FLinearColor::Blue(0, 0, 1);
const FLinearColor FLinearColor::Yellow(1, 1, 0);
const FColor FColor::White(255, 255, 255);
const FColor FColor::Black(0, 0, 0);
const FColor FColor::Transparent(0, 0, 0, 0);

namespace {

float SRGBToLinear(uint8 Value) {
    const float C = Value / 255.0f;
    return C <= 0.04045f ? C / 12.92f : std::pow((C + 0.055f) / 1.055f, 2.4f);
}

}

FLinearColor::FLinearColor(const FColor& Color)
    : R(SRGBToLinear(Color.R)), G(SRGBToLinear(Color.G)), B(SRGBToLinear(Color.B)), A(Color.A / 255.0f) {}

FLinearColor FLinearColor::FromSRGBColor(const FColor& Color) { return FLinearColor(Color); }

FLinearColor FLinearColor::FromPow22Color(const FColor& Color) {
    return FLinearColor(std::pow(Color.R / 255.0f, 2.2f), std::pow(Color.G / 255.0f, 2.2f), std::pow(Color.B / 255.0f, 2.2f), Color.A / 255.0f);
}

FColor FLinearColor::ToFColor(bool bSRGB) const {
    float FloatR = FMath::Clamp(R, 0.0f, 1.0f);
    float FloatG = FMath::Clamp(G, 0.0f, 1.0f);
    float FloatB = FMath::Clamp(B, 0.0f, 1.0f);
    float FloatA = FMath::Clamp(A, 0.0f, 1.0f);
    if (bSRGB) {
        auto Encode = [](float C) { return C <= 0.0031308f ? C * 12.92f : std::pow(C, 1.0f / 2.4f) * 1.055f - 0.055f; };
        FloatR = Encode(FloatR);
        FloatG = Encode(FloatG);
        FloatB = Encode(FloatB);
    }
    FColor Result;
    Result.A = static_cast<uint8>(FMath::FloorToInt(FloatA * 255.999f));
    Result.R = static_cast<uint8>(FMath::FloorToInt(FloatR * 255.999f));
    Result.G = static_cast<uint8>(FMath::FloorToInt(FloatG * 255.999f));
    Result.B = static_cast<uint8>(FMath::FloorToInt(FloatB * 255.999f));
    return Result;
}

FColor FLinearColor::Quantize() const {
    auto Q = [](float C) { return static_cast<uint8>(FMath::Clamp<int32>(FMath::TruncToInt(C * 255.f), 0, 255)); };
    return FColor(Q(R), Q(G), Q(B), Q(A));
}

FColor FLinearColor::QuantizeRound() const {
    auto Q = [](float C) { return static_cast<uint8>(FMath::Clamp<int32>(FMath::RoundToInt(C * 255.f), 0, 255)); };
    return FColor(Q(R), Q(G), Q(B), Q(A));
}

FLinearColor FLinearColor::LinearRGBToHSV() const {
    const float RGBMin = FMath::Min3(R, G, B);
    const float RGBMax = FMath::Max3(R, G, B);
    const float RGBRange = RGBMax - RGBMin;
    const float Hue = RGBMax == RGBMin ? 0.0f
                      : RGBMax == R    ? FMath::Fmod((((G - B) / RGBRange) * 60.0f) + 360.0f, 360.0f)
                      : RGBMax == G    ? (((B - R) / RGBRange) * 60.0f) + 120.0f
                      : RGBMax == B    ? (((R - G) / RGBRange) * 60.0f) + 240.0f
                                       : 0.0f;
    const float Saturation = RGBMax == 0.0f ? 0.0f : RGBRange / RGBMax;
    return FLinearColor(Hue, Saturation, RGBMax, A);
}

FLinearColor FLinearColor::HSVToLinearRGB() const {
    const float Hue = R, Saturation = G, Value = B;
    const float HDiv60 = Hue / 60.0f;
    const float HDiv60Floor = std::floor(HDiv60);
    const float HDiv60Fraction = HDiv60 - HDiv60Floor;
    const float RGBValues[4] = {Value, Value * (1.0f - Saturation), Value * (1.0f - (HDiv60Fraction * Saturation)),
                                Value * (1.0f - ((1.0f - HDiv60Fraction) * Saturation))};
    static constexpr uint32 RGBSwizzle[6][3] = {{0, 3, 1}, {2, 0, 1}, {1, 0, 3}, {1, 2, 0}, {3, 1, 0}, {0, 1, 2}};
    const uint32 SwizzleIndex = static_cast<uint32>(HDiv60Floor) % 6;
    return FLinearColor(RGBValues[RGBSwizzle[SwizzleIndex][0]], RGBValues[RGBSwizzle[SwizzleIndex][1]], RGBValues[RGBSwizzle[SwizzleIndex][2]], A);
}

FLinearColor FLinearColor::MakeFromHSV8(uint8 H, uint8 S, uint8 V) {
    return FLinearColor(H * 360.0f / 255.0f, S / 255.0f, V / 255.0f).HSVToLinearRGB();
}

FLinearColor FLinearColor::LerpUsingHSV(const FLinearColor& From, const FLinearColor& To, float Progress) {
    const FLinearColor FromHSV = From.LinearRGBToHSV();
    const FLinearColor ToHSV = To.LinearRGBToHSV();
    float FromHue = FromHSV.R;
    float ToHue = ToHSV.R;
    if (FMath::Abs(FromHue - ToHue) > 180.0f) {
        if (ToHue > FromHue) FromHue += 360.0f;
        else ToHue += 360.0f;
    }
    float NewHue = FMath::Lerp(FromHue, ToHue, Progress);
    NewHue = FMath::Fmod(NewHue, 360.0f);
    if (NewHue < 0.0f) NewHue += 360.0f;
    const float NewSaturation = FMath::Lerp(FromHSV.G, ToHSV.G, Progress);
    const float NewValue = FMath::Lerp(FromHSV.B, ToHSV.B, Progress);
    FLinearColor Interpolated = FLinearColor(NewHue, NewSaturation, NewValue).HSVToLinearRGB();
    Interpolated.A = FMath::Lerp(From.A, To.A, Progress);
    return Interpolated;
}

FLinearColor FLinearColor::MakeFromColorTemperature(float Temp) {
    Temp = FMath::Clamp(Temp, 1000.0f, 15000.0f);
    // The Planckian locus in CIE 1960 UCS, then XYZ to linear sRGB.
    const float U = (0.860117757f + 1.54118254e-4f * Temp + 1.28641212e-7f * Temp * Temp) /
                    (1.0f + 8.42420235e-4f * Temp + 7.08145163e-7f * Temp * Temp);
    const float V = (0.317398726f + 4.22806245e-5f * Temp + 4.20481691e-8f * Temp * Temp) /
                    (1.0f - 2.89741816e-5f * Temp + 1.61456053e-7f * Temp * Temp);
    const float X = 3.0f * U / (2.0f * U - 8.0f * V + 4.0f);
    const float Yc = 2.0f * V / (2.0f * U - 8.0f * V + 4.0f);
    const float Zc = 1.0f - X - Yc;
    const float Y = 1.0f;
    const float XX = Y / Yc * X;
    const float ZZ = Y / Yc * Zc;
    const float Red = 3.2404542f * XX + -1.5371385f * Y + -0.4985314f * ZZ;
    const float Green = -0.9692660f * XX + 1.8760108f * Y + 0.0415560f * ZZ;
    const float Blue = 0.0556434f * XX + -0.2040259f * Y + 1.0572252f * ZZ;
    return FLinearColor(Red, Green, Blue);
}

template struct TVector<double>;
template struct TVector<float>;
template struct TRotator<double>;
template struct TRotator<float>;
template struct TQuat<double>;
template struct TQuat<float>;
template struct TMatrix<double>;
template struct TMatrix<float>;
template TMatrix<double> MakeRotationTranslationMatrix(const TRotator<double>&, const TVector<double>&);
template TMatrix<float> MakeRotationTranslationMatrix(const TRotator<float>&, const TVector<float>&);
template TMatrix<double> MakeQuatRotationTranslationMatrix(const TQuat<double>&, const TVector<double>&);
template TMatrix<float> MakeQuatRotationTranslationMatrix(const TQuat<float>&, const TVector<float>&);

}

// --- RandomStream ---

namespace ue5parse::runtime {

// A direction uniform on the sphere (theta around, phi from the pole), its polar angle folded into the cone, then
// turned from +X onto Dir.
FVector FRandomStream::VRandCone(const FVector& Dir, float ConeHalfAngleRad) const {
    if (ConeHalfAngleRad <= 0.f) return Dir.GetSafeNormal();
    const float RandU = FRand();
    const float RandV = FRand();
    const double Theta = 2.0 * UE_PI * RandU;
    double Phi = std::acos(2.0 * RandV - 1.0);
    Phi = FMath::Fmod(Phi, static_cast<double>(ConeHalfAngleRad));
    const FMatrix DirMat = FRotationMatrix(Dir.Rotation());
    const FVector DirZ = DirMat.GetScaledAxis(0);
    const FVector DirY = DirMat.GetScaledAxis(1);
    FVector Result = Dir.RotateAngleAxis(Phi * 180.0 / UE_PI, DirY);
    Result = Result.RotateAngleAxis(Theta * 180.0 / UE_PI, DirZ);
    return Result.GetSafeNormal();
}

}
