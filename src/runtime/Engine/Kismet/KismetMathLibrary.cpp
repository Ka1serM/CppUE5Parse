#include "ue5parse/runtime/Engine/Kismet/KismetMathLibrary.h"

#include "ue5parse/runtime/CoreUObject/UObject/Stack.h"

namespace ue5parse::runtime {

namespace {

// A problem of the calling script: UE reports it on the current frame and returns a neutral value.
void ReportMathError(std::string_view Message) {
    if (const FFrame* Frame = FFrame::GetThreadLocalTopStackFrame()) Frame->KismetExecutionMessage("KismetMath", Message);
}

double EaseAlpha(double Alpha, EEasingFunc::Type EasingFunc, double BlendExp, int32 Steps) {
    switch (EasingFunc) {
    case EEasingFunc::Step: return FMath::InterpStep(0.0, 1.0, Alpha, Steps);
    case EEasingFunc::SinusoidalIn: return FMath::InterpSinIn(0.0, 1.0, Alpha);
    case EEasingFunc::SinusoidalOut: return FMath::InterpSinOut(0.0, 1.0, Alpha);
    case EEasingFunc::SinusoidalInOut: return FMath::InterpSinInOut(0.0, 1.0, Alpha);
    case EEasingFunc::EaseIn: return FMath::InterpEaseIn(0.0, 1.0, Alpha, BlendExp);
    case EEasingFunc::EaseOut: return FMath::InterpEaseOut(0.0, 1.0, Alpha, BlendExp);
    case EEasingFunc::EaseInOut: return FMath::InterpEaseInOut(0.0, 1.0, Alpha, BlendExp);
    case EEasingFunc::ExpoIn: return FMath::InterpExpoIn(0.0, 1.0, Alpha);
    case EEasingFunc::ExpoOut: return FMath::InterpExpoOut(0.0, 1.0, Alpha);
    case EEasingFunc::ExpoInOut: return FMath::InterpExpoInOut(0.0, 1.0, Alpha);
    case EEasingFunc::CircularIn: return FMath::InterpCircularIn(0.0, 1.0, Alpha);
    case EEasingFunc::CircularOut: return FMath::InterpCircularOut(0.0, 1.0, Alpha);
    case EEasingFunc::CircularInOut: return FMath::InterpCircularInOut(0.0, 1.0, Alpha);
    default: return Alpha;
    }
}

// A uniform unit vector: a point in the unit ball, normalized (UE's FMath::VRand).
FVector RandomUnitVectorFromGlobalStream() {
    FVector Result;
    double L;
    do {
        Result = FVector(FMath::FRand() * 2.0 - 1.0, FMath::FRand() * 2.0 - 1.0, FMath::FRand() * 2.0 - 1.0);
        L = Result.SizeSquared();
    } while (L > 1.0 || L < UE_KINDA_SMALL_NUMBER);
    return Result * (1.0 / std::sqrt(L));
}

FVector ClosestPointOnSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint) {
    const FVector Segment = EndPoint - StartPoint;
    const FVector VectToPoint = Point - StartPoint;
    const double Dot1 = VectToPoint | Segment;
    if (Dot1 <= 0) return StartPoint;
    const double Dot2 = Segment | Segment;
    if (Dot2 <= Dot1) return EndPoint;
    return StartPoint + Segment * (Dot1 / Dot2);
}

template <class T>
bool InRange(T Value, T Min, T Max, bool InclusiveMin, bool InclusiveMax) {
    return (InclusiveMin ? Value >= Min : Value > Min) && (InclusiveMax ? Value <= Max : Value < Max);
}

// A rotation and a translation as a dual quaternion: the real part is the rotation, the dual part half the
// translation times the rotation (UE's FDualQuat).
struct FDualQuat {
    FQuat R;
    FQuat D;

    explicit FDualQuat(const FTransform& Transform) : R(Transform.GetRotation()) {
        const FVector T = Transform.GetTranslation();
        D = FQuat(T.X, T.Y, T.Z, 0.0) * R;
        D = FQuat(D.X * 0.5, D.Y * 0.5, D.Z * 0.5, D.W * 0.5);
    }
    FDualQuat(const FQuat& InR, const FQuat& InD) : R(InR), D(InD) {}
    FDualQuat operator*(double S) const { return {FQuat(R.X * S, R.Y * S, R.Z * S, R.W * S), FQuat(D.X * S, D.Y * S, D.Z * S, D.W * S)}; }
    FDualQuat operator+(const FDualQuat& O) const {
        return {FQuat(R.X + O.R.X, R.Y + O.R.Y, R.Z + O.R.Z, R.W + O.R.W), FQuat(D.X + O.D.X, D.Y + O.D.Y, D.Z + O.D.Z, D.W + O.D.W)};
    }
    FDualQuat Normalized() const {
        const double MinV = 1.0e-4;
        const double Size = std::sqrt(R.X * R.X + R.Y * R.Y + R.Z * R.Z + R.W * R.W);
        return *this * (1.0 / (Size > MinV ? Size : MinV));
    }
    FTransform AsFTransform(const FVector& Scale) const {
        const FQuat TQ = D * FQuat(-R.X, -R.Y, -R.Z, R.W);
        return FTransform(R, FVector(TQ.X * 2.0, TQ.Y * 2.0, TQ.Z * 2.0), Scale);
    }
};

}

// --- Bool ---

bool UKismetMathLibrary::RandomBool() { return FMath::RandBool(); }
bool UKismetMathLibrary::RandomBoolWithWeight(float Weight) { return Weight > 0.f && FMath::FRandRange(0.0, 1.0) <= Weight; }
bool UKismetMathLibrary::RandomBoolWithWeightFromStream(const FRandomStream& RandomStream, float Weight) {
    return Weight > 0.f && RandomStream.FRandRange(0.f, 1.f) <= Weight;
}
bool UKismetMathLibrary::Not_PreBool(bool A) { return !A; }
bool UKismetMathLibrary::EqualEqual_BoolBool(bool A, bool B) { return A == B; }
bool UKismetMathLibrary::NotEqual_BoolBool(bool A, bool B) { return A != B; }
bool UKismetMathLibrary::BooleanAND(bool A, bool B) { return A && B; }
bool UKismetMathLibrary::BooleanNAND(bool A, bool B) { return !(A && B); }
bool UKismetMathLibrary::BooleanOR(bool A, bool B) { return A || B; }
bool UKismetMathLibrary::BooleanXOR(bool A, bool B) { return A ^ B; }
bool UKismetMathLibrary::BooleanNOR(bool A, bool B) { return !(A || B); }

// --- Byte ---

bool UKismetMathLibrary::Less_ByteByte(uint8 A, uint8 B) { return A < B; }
bool UKismetMathLibrary::Greater_ByteByte(uint8 A, uint8 B) { return A > B; }
bool UKismetMathLibrary::LessEqual_ByteByte(uint8 A, uint8 B) { return A <= B; }
bool UKismetMathLibrary::GreaterEqual_ByteByte(uint8 A, uint8 B) { return A >= B; }
bool UKismetMathLibrary::EqualEqual_ByteByte(uint8 A, uint8 B) { return A == B; }
bool UKismetMathLibrary::NotEqual_ByteByte(uint8 A, uint8 B) { return A != B; }

// --- Integer ---

int32 UKismetMathLibrary::Multiply_IntInt(int32 A, int32 B) { return A * B; }
int32 UKismetMathLibrary::Divide_IntInt(int32 A, int32 B) {
    if (B != 0) return A / B;
    ReportMathError("Divide by zero: Divide_IntInt");
    return 0;
}
int32 UKismetMathLibrary::Percent_IntInt(int32 A, int32 B) {
    if (B != 0) return A % B;
    ReportMathError("Modulo by zero");
    return 0;
}
int32 UKismetMathLibrary::Add_IntInt(int32 A, int32 B) { return A + B; }
int32 UKismetMathLibrary::Subtract_IntInt(int32 A, int32 B) { return A - B; }
bool UKismetMathLibrary::Less_IntInt(int32 A, int32 B) { return A < B; }
bool UKismetMathLibrary::Greater_IntInt(int32 A, int32 B) { return A > B; }
bool UKismetMathLibrary::LessEqual_IntInt(int32 A, int32 B) { return A <= B; }
bool UKismetMathLibrary::GreaterEqual_IntInt(int32 A, int32 B) { return A >= B; }
bool UKismetMathLibrary::EqualEqual_IntInt(int32 A, int32 B) { return A == B; }
bool UKismetMathLibrary::NotEqual_IntInt(int32 A, int32 B) { return A != B; }
bool UKismetMathLibrary::InRange_IntInt(int32 Value, int32 Min, int32 Max, bool InclusiveMin, bool InclusiveMax) {
    return InRange(Value, Min, Max, InclusiveMin, InclusiveMax);
}
int32 UKismetMathLibrary::And_IntInt(int32 A, int32 B) { return A & B; }
int32 UKismetMathLibrary::SignOfInteger(int32 A) { return FMath::Sign(A); }
int32 UKismetMathLibrary::RandomInteger(int32 Max) { return FMath::RandHelper(Max); }
int32 UKismetMathLibrary::RandomIntegerInRange(int32 Min, int32 Max) { return FMath::RandRange(Min, Max); }
int32 UKismetMathLibrary::Min(int32 A, int32 B) { return FMath::Min(A, B); }
int32 UKismetMathLibrary::Max(int32 A, int32 B) { return FMath::Max(A, B); }
int32 UKismetMathLibrary::Clamp(int32 Value, int32 Min, int32 Max) { return FMath::Clamp(Value, Min, Max); }
int32 UKismetMathLibrary::Abs_Int(int32 A) { return FMath::Abs(A); }
bool UKismetMathLibrary::Greater_Int64Int64(int64 A, int64 B) { return A > B; }
bool UKismetMathLibrary::EqualEqual_Int64Int64(int64 A, int64 B) { return A == B; }

// --- Float ---

double UKismetMathLibrary::MultiplyMultiply_FloatFloat(double Base, double Exp) { return FMath::Pow(Base, Exp); }
double UKismetMathLibrary::Multiply_IntFloat(int32 A, double B) { return A * B; }
double UKismetMathLibrary::Percent_FloatFloat(double A, double B) {
    if (B != 0.0) return FMath::Fmod(A, B);
    ReportMathError("Modulo by zero");
    return 0.0;
}
double UKismetMathLibrary::Fraction(double A) { return FMath::Fractional(A); }
double UKismetMathLibrary::Add_DoubleDouble(double A, double B) { return A + B; }
double UKismetMathLibrary::Subtract_DoubleDouble(double A, double B) { return A - B; }
double UKismetMathLibrary::Multiply_DoubleDouble(double A, double B) { return A * B; }
double UKismetMathLibrary::Divide_DoubleDouble(double A, double B) {
    if (B != 0.0) return A / B;
    ReportMathError("Divide by zero: Divide_DoubleDouble");
    return 0.0;
}
bool UKismetMathLibrary::Less_DoubleDouble(double A, double B) { return A < B; }
bool UKismetMathLibrary::Greater_DoubleDouble(double A, double B) { return A > B; }
bool UKismetMathLibrary::LessEqual_DoubleDouble(double A, double B) { return A <= B; }
bool UKismetMathLibrary::GreaterEqual_DoubleDouble(double A, double B) { return A >= B; }
bool UKismetMathLibrary::EqualEqual_DoubleDouble(double A, double B) { return A == B; }
bool UKismetMathLibrary::NearlyEqual_FloatFloat(double A, double B, double ErrorTolerance) { return FMath::IsNearlyEqual(A, B, ErrorTolerance); }
bool UKismetMathLibrary::NotEqual_DoubleDouble(double A, double B) { return A != B; }
bool UKismetMathLibrary::InRange_FloatFloat(double Value, double Min, double Max, bool InclusiveMin, bool InclusiveMax) {
    return InRange(Value, Min, Max, InclusiveMin, InclusiveMax);
}
double UKismetMathLibrary::GridSnap_Float(double Location, double GridSize) {
    if (GridSize == 0.0) return Location;
    return FMath::FloorToDouble((Location + 0.5 * GridSize) / GridSize) * GridSize;
}
double UKismetMathLibrary::Abs(double A) { return FMath::Abs(A); }
double UKismetMathLibrary::Sin(double A) { return std::sin(A); }
double UKismetMathLibrary::Exp(double A) { return std::exp(A); }
double UKismetMathLibrary::Sqrt(double A) {
    if (A >= 0.0) return std::sqrt(A);
    ReportMathError("Attempt to take Sqrt() of negative number - returning 0.");
    return 0.0;
}
double UKismetMathLibrary::Square(double A) { return A * A; }
double UKismetMathLibrary::RandomFloat() { return FMath::FRand(); }
double UKismetMathLibrary::RandomFloatInRange(double Min, double Max) { return FMath::FRandRange(Min, Max); }
double UKismetMathLibrary::GetPI() { return UE_PI; }
double UKismetMathLibrary::DegreesToRadians(double A) { return FMath::DegreesToRadians(A); }
double UKismetMathLibrary::RadiansToDegrees(double A) { return FMath::RadiansToDegrees(A); }
double UKismetMathLibrary::DegSin(double A) { return std::sin(UE_PI / 180.0 * A); }
double UKismetMathLibrary::DegAsin(double A) { return 180.0 / UE_PI * std::asin(A); }
double UKismetMathLibrary::DegCos(double A) { return std::cos(UE_PI / 180.0 * A); }
double UKismetMathLibrary::DegAcos(double A) { return 180.0 / UE_PI * std::acos(A); }
double UKismetMathLibrary::DegTan(double A) { return std::tan(UE_PI / 180.0 * A); }
double UKismetMathLibrary::DegAtan(double A) { return 180.0 / UE_PI * std::atan(A); }
double UKismetMathLibrary::DegAtan2(double Y, double X) { return 180.0 / UE_PI * std::atan2(Y, X); }
double UKismetMathLibrary::ClampAngle(double AngleDegrees, double MinAngleDegrees, double MaxAngleDegrees) {
    return FMath::ClampAngle(AngleDegrees, MinAngleDegrees, MaxAngleDegrees);
}
double UKismetMathLibrary::FMin(double A, double B) { return FMath::Min(A, B); }
double UKismetMathLibrary::FMax(double A, double B) { return FMath::Max(A, B); }
double UKismetMathLibrary::FClamp(double Value, double Min, double Max) { return FMath::Clamp(Value, Min, Max); }
double UKismetMathLibrary::FWrap(double Value, double Min, double Max) { return FMath::Wrap(Value, Min, Max); }
double UKismetMathLibrary::SafeDivide(double A, double B) { return B != 0.0 ? A / B : 0.0; }

// The first largest element, INDEX_NONE and zero for an empty array (UE's FMath::Max of an array).
void UKismetMathLibrary::MaxOfIntArray(const TArray<int32>& IntArray, int32& IndexOfMaxValue, int32& MaxValue) {
    IndexOfMaxValue = INDEX_NONE;
    MaxValue = 0;
    for (int32 Index = 0; Index < IntArray.Num(); Index++) {
        if (IndexOfMaxValue != INDEX_NONE && IntArray[Index] <= MaxValue) continue;
        IndexOfMaxValue = Index;
        MaxValue = IntArray[Index];
    }
}
void UKismetMathLibrary::MaxOfFloatArray(const TArray<float>& FloatArray, int32& IndexOfMaxValue, float& MaxValue) {
    IndexOfMaxValue = INDEX_NONE;
    MaxValue = 0.f;
    for (int32 Index = 0; Index < FloatArray.Num(); Index++) {
        if (IndexOfMaxValue != INDEX_NONE && FloatArray[Index] <= MaxValue) continue;
        IndexOfMaxValue = Index;
        MaxValue = FloatArray[Index];
    }
}

double UKismetMathLibrary::Lerp(double A, double B, double Alpha) { return A + Alpha * (B - A); }
double UKismetMathLibrary::Ease(double A, double B, double Alpha, TEnumAsByte<EEasingFunc::Type> EasingFunc, double BlendExp, int32 Steps) {
    return Lerp(A, B, EaseAlpha(Alpha, EasingFunc, BlendExp, Steps));
}
int32 UKismetMathLibrary::Round(double A) { return FMath::RoundToInt(A); }
int32 UKismetMathLibrary::FFloor(double A) { return FMath::FloorToInt(A); }
int32 UKismetMathLibrary::FTrunc(double A) { return FMath::TruncToInt(A); }
int32 UKismetMathLibrary::FCeil(double A) { return FMath::CeilToInt(A); }
int64 UKismetMathLibrary::FTrunc64(double A) { return FMath::TruncToInt64(A); }
int32 UKismetMathLibrary::FMod(double Dividend, double Divisor, double& Remainder) {
    if (Divisor == 0.0) {
        ReportMathError("Attempted modulo 0 - returning 0.");
        Remainder = 0.0;
        return 0;
    }
    const double Quotient = Dividend / Divisor;
    Remainder = FMath::Fmod(Dividend, Divisor);
    return (Quotient < 0.0 ? -1 : 1) * FMath::FloorToInt(FMath::Abs(Quotient));
}
double UKismetMathLibrary::SignOfFloat(double A) { return FMath::Sign(A); }
double UKismetMathLibrary::NormalizeToRange(double Value, double RangeMin, double RangeMax) {
    if (RangeMin == RangeMax) return Value < RangeMin ? 0.0 : 1.0;
    if (RangeMin > RangeMax) std::swap(RangeMin, RangeMax);
    return (Value - RangeMin) / (RangeMax - RangeMin);
}
double UKismetMathLibrary::MapRangeUnclamped(double Value, double InRangeA, double InRangeB, double OutRangeA, double OutRangeB) {
    return FMath::GetMappedRangeValueUnclamped(InRangeA, InRangeB, OutRangeA, OutRangeB, Value);
}
double UKismetMathLibrary::MapRangeClamped(double Value, double InRangeA, double InRangeB, double OutRangeA, double OutRangeB) {
    return FMath::GetMappedRangeValueClamped(InRangeA, InRangeB, OutRangeA, OutRangeB, Value);
}
double UKismetMathLibrary::MultiplyByPi(double Value) { return Value * UE_PI; }
double UKismetMathLibrary::FInterpTo(double Current, double Target, double DeltaTime, double InterpSpeed) {
    return FMath::FInterpTo(Current, Target, DeltaTime, InterpSpeed);
}
double UKismetMathLibrary::FInterpTo_Constant(double Current, double Target, double DeltaTime, double InterpSpeed) {
    return FMath::FInterpConstantTo(Current, Target, DeltaTime, InterpSpeed);
}

// --- Vector2D ---

bool UKismetMathLibrary::Equal_IntPointIntPoint(FIntPoint A, FIntPoint B) { return A == B; }
FVector2D UKismetMathLibrary::MakeVector2D(double X, double Y) { return FVector2D(X, Y); }
void UKismetMathLibrary::BreakVector2D(FVector2D InVec, double& X, double& Y) {
    X = InVec.X;
    Y = InVec.Y;
}
FVector UKismetMathLibrary::Conv_Vector2DToVector(FVector2D InVector2D, float Z) { return FVector(InVector2D.X, InVector2D.Y, Z); }
FVector2D UKismetMathLibrary::Add_Vector2DVector2D(FVector2D A, FVector2D B) { return A + B; }
FVector2D UKismetMathLibrary::Subtract_Vector2DVector2D(FVector2D A, FVector2D B) { return A - B; }
FVector2D UKismetMathLibrary::Multiply_Vector2DVector2D(FVector2D A, FVector2D B) { return A * B; }
FVector2D UKismetMathLibrary::Multiply_Vector2DFloat(FVector2D A, double B) { return A * B; }
bool UKismetMathLibrary::EqualEqual_Vector2DVector2D(FVector2D A, FVector2D B, float ErrorTolerance) { return A.Equals(B, ErrorTolerance); }
bool UKismetMathLibrary::NotEqual_Vector2DVector2D(FVector2D A, FVector2D B, float ErrorTolerance) { return !A.Equals(B, ErrorTolerance); }
double UKismetMathLibrary::Distance2D(FVector2D V1, FVector2D V2) { return FVector2D::Distance(V1, V2); }
double UKismetMathLibrary::DistanceSquared2D(FVector2D V1, FVector2D V2) { return FVector2D::DistSquared(V1, V2); }
double UKismetMathLibrary::DotProduct2D(FVector2D A, FVector2D B) { return A | B; }
FVector2D UKismetMathLibrary::GetRotated2D(FVector2D A, float AngleDeg) { return A.GetRotated(AngleDeg); }
bool UKismetMathLibrary::IsNearlyZero2D(const FVector2D& A, float Tolerance) { return A.IsNearlyZero(Tolerance); }
FVector2D UKismetMathLibrary::NormalSafe2D(FVector2D A, float Tolerance) { return A.GetSafeNormal(Tolerance); }
FVector2D UKismetMathLibrary::Normal2D(FVector2D A) { return A.GetSafeNormal(); }
double UKismetMathLibrary::VSize2D(FVector2D A) { return A.Size(); }
double UKismetMathLibrary::VSize2DSquared(FVector2D A) { return A.SizeSquared(); }
FVector2D UKismetMathLibrary::Conv_VectorToVector2D(FVector InVector) { return FVector2D(InVector.X, InVector.Y); }
FVector2D UKismetMathLibrary::Conv_DoubleToVector2D(double InDouble) { return FVector2D(InDouble); }

// --- Vector ---

FVector UKismetMathLibrary::Vector_Zero() { return FVector::ZeroVector; }
FVector UKismetMathLibrary::Vector_Up() { return FVector::UpVector; }
FVector UKismetMathLibrary::MakeVector(double X, double Y, double Z) { return FVector(X, Y, Z); }
FVector UKismetMathLibrary::CreateVectorFromYawPitch(float Yaw, float Pitch, float Length) { return FRotator(Pitch, Yaw, 0).Vector() * Length; }
void UKismetMathLibrary::Vector_Set(FVector& A, double X, double Y, double Z) { A = FVector(X, Y, Z); }
void UKismetMathLibrary::BreakVector(FVector InVec, double& X, double& Y, double& Z) {
    X = InVec.X;
    Y = InVec.Y;
    Z = InVec.Z;
}
FLinearColor UKismetMathLibrary::Conv_VectorToLinearColor(FVector InVec) { return FLinearColor(InVec); }
FTransform UKismetMathLibrary::Conv_VectorToTransform(FVector InLocation) { return FTransform(InLocation); }
FRotator UKismetMathLibrary::Conv_VectorToRotator(FVector InVec) { return InVec.Rotation(); }
FRotator UKismetMathLibrary::RotatorFromAxisAndAngle(FVector Axis, float Angle) {
    return FQuat(Axis.GetSafeNormal(), FMath::DegreesToRadians(static_cast<double>(Angle))).Rotator();
}
FQuat UKismetMathLibrary::Conv_VectorToQuaternion(FVector InVec) { return InVec.ToOrientationQuat(); }
FVector UKismetMathLibrary::Add_VectorVector(FVector A, FVector B) { return A + B; }
FVector UKismetMathLibrary::Add_VectorFloat(FVector A, double B) { return A + B; }
FVector UKismetMathLibrary::Subtract_VectorVector(FVector A, FVector B) { return A - B; }
FVector UKismetMathLibrary::Subtract_VectorFloat(FVector A, double B) { return A - B; }
FVector UKismetMathLibrary::Multiply_VectorVector(FVector A, FVector B) { return A * B; }
FVector UKismetMathLibrary::Multiply_VectorFloat(FVector A, double B) { return A * B; }
FVector UKismetMathLibrary::Multiply_VectorInt(FVector A, int32 B) { return A * static_cast<double>(B); }
FVector UKismetMathLibrary::Divide_VectorVector(FVector A, FVector B) {
    if (B.X != 0.0 && B.Y != 0.0 && B.Z != 0.0) return A / B;
    ReportMathError("Divide by zero: Divide_VectorVector");
    return FVector::ZeroVector;
}
FVector UKismetMathLibrary::Divide_VectorFloat(FVector A, double B) {
    if (B != 0.0) return A / B;
    ReportMathError("Divide by zero: Divide_VectorFloat");
    return FVector::ZeroVector;
}
FVector UKismetMathLibrary::Divide_VectorInt(FVector A, int32 B) {
    if (B != 0) return A / static_cast<double>(B);
    ReportMathError("Divide by zero: Divide_VectorInt");
    return FVector::ZeroVector;
}
FVector UKismetMathLibrary::NegateVector(FVector A) { return -A; }
bool UKismetMathLibrary::EqualExactly_VectorVector(FVector A, FVector B) { return A == B; }
bool UKismetMathLibrary::EqualEqual_VectorVector(FVector A, FVector B, float ErrorTolerance) { return A.Equals(B, ErrorTolerance); }
bool UKismetMathLibrary::NotEqualExactly_VectorVector(FVector A, FVector B) { return !(A == B); }
bool UKismetMathLibrary::NotEqual_VectorVector(FVector A, FVector B, float ErrorTolerance) { return !A.Equals(B, ErrorTolerance); }
double UKismetMathLibrary::Dot_VectorVector(FVector A, FVector B) { return A | B; }
FVector UKismetMathLibrary::Cross_VectorVector(FVector A, FVector B) { return A ^ B; }
FVector UKismetMathLibrary::GreaterGreater_VectorRotator(FVector A, FRotator B) { return B.RotateVector(A); }
FVector UKismetMathLibrary::RotateAngleAxis(FVector InVect, float AngleDeg, FVector Axis) {
    return InVect.RotateAngleAxis(AngleDeg, Axis.GetSafeNormal());
}
FVector UKismetMathLibrary::LessLess_VectorRotator(FVector A, FRotator B) { return B.UnrotateVector(A); }
FVector UKismetMathLibrary::ClampVectorSize(FVector A, double Min, double Max) { return A.GetClampedToSize(Min, Max); }
FVector UKismetMathLibrary::Vector_ClampSizeMax(FVector A, double Max) { return A.GetClampedToMaxSize(Max); }
FVector UKismetMathLibrary::Vector_GetAbs(FVector A) { return A.GetAbs(); }
FVector UKismetMathLibrary::Vector_ComponentMin(FVector A, FVector B) { return A.ComponentMin(B); }
FVector UKismetMathLibrary::Vector_ComponentMax(FVector A, FVector B) { return A.ComponentMax(B); }
double UKismetMathLibrary::Vector_HeadingAngle(FVector A) { return A.HeadingAngle(); }
double UKismetMathLibrary::Vector_CosineAngle2D(FVector A, FVector B) { return A.CosineAngle2D(B); }
FVector UKismetMathLibrary::GetDirectionUnitVector(FVector From, FVector To) { return (To - From).GetSafeNormal(); }
void UKismetMathLibrary::GetYawPitchFromVector(FVector InVec, float& Yaw, float& Pitch) {
    const FRotator Rotator = InVec.ToOrientationRotator();
    Yaw = static_cast<float>(Rotator.Yaw);
    Pitch = static_cast<float>(Rotator.Pitch);
}
FVector UKismetMathLibrary::GetVectorArrayAverage(const TArray<FVector>& Vectors) {
    FVector Sum = FVector::ZeroVector;
    for (const FVector& Vector : Vectors) Sum += Vector;
    return Vectors.Num() > 0 ? Sum / static_cast<double>(Vectors.Num()) : Sum;
}
double UKismetMathLibrary::Vector_Distance(FVector V1, FVector V2) { return FVector::Dist(V1, V2); }
double UKismetMathLibrary::Vector_DistanceSquared(FVector V1, FVector V2) { return FVector::DistSquared(V1, V2); }
double UKismetMathLibrary::Vector_Distance2D(FVector V1, FVector V2) { return FVector::Dist2D(V1, V2); }
double UKismetMathLibrary::Vector_Distance2DSquared(FVector V1, FVector V2) { return FVector::DistSquared2D(V1, V2); }
double UKismetMathLibrary::VSize(FVector A) { return A.Size(); }
double UKismetMathLibrary::VSizeSquared(FVector A) { return A.SizeSquared(); }
double UKismetMathLibrary::VSizeXY(FVector A) { return A.Size2D(); }
double UKismetMathLibrary::VSizeXYSquared(FVector A) { return A.SizeSquared2D(); }
bool UKismetMathLibrary::Vector_IsNearlyZero(const FVector& A, float Tolerance) { return A.IsNearlyZero(Tolerance); }
bool UKismetMathLibrary::Vector_IsZero(const FVector& A) { return A.IsZero(); }
bool UKismetMathLibrary::Vector_IsNAN(const FVector& A) { return std::isnan(A.X) || std::isnan(A.Y) || std::isnan(A.Z); }
FVector UKismetMathLibrary::Normal(FVector A, float Tolerance) { return A.GetSafeNormal(Tolerance); }
FVector UKismetMathLibrary::Vector_Normal2D(FVector A, float Tolerance) { return A.GetSafeNormal2D(Tolerance); }
FVector UKismetMathLibrary::VLerp(FVector A, FVector B, float Alpha) { return A + (B - A) * static_cast<double>(Alpha); }
FVector UKismetMathLibrary::VEase(FVector A, FVector B, float Alpha, TEnumAsByte<EEasingFunc::Type> EasingFunc, float BlendExp, int32 Steps) {
    return A + (B - A) * EaseAlpha(Alpha, EasingFunc, BlendExp, Steps);
}
FVector UKismetMathLibrary::VInterpTo(FVector Current, FVector Target, float DeltaTime, float InterpSpeed) {
    if (InterpSpeed <= 0.f) return Target;
    const FVector Dist = Target - Current;
    if (Dist.SizeSquared() < UE_KINDA_SMALL_NUMBER) return Target;
    return Current + Dist * FMath::Clamp(static_cast<double>(DeltaTime) * InterpSpeed, 0.0, 1.0);
}
FVector UKismetMathLibrary::VInterpTo_Constant(FVector Current, FVector Target, float DeltaTime, float InterpSpeed) {
    const FVector Delta = Target - Current;
    const double DeltaM = Delta.Size();
    const double MaxStep = static_cast<double>(InterpSpeed) * DeltaTime;
    if (DeltaM <= MaxStep) return Target;
    if (MaxStep <= 0.0) return Current;
    return Current + Delta / DeltaM * MaxStep;
}
FVector UKismetMathLibrary::GetReflectionVector(FVector Direction, FVector SurfaceNormal) {
    const FVector Normal = SurfaceNormal.GetSafeNormal();
    return Direction - Normal * (2.0 * (Direction | Normal));
}
FVector UKismetMathLibrary::MirrorVectorByNormal(FVector InVect, FVector InNormal) { return InVect.MirrorByVector(InNormal); }
FVector UKismetMathLibrary::Vector_ProjectOnToNormal(FVector V, FVector InNormal) { return V.ProjectOnToNormal(InNormal); }
FVector UKismetMathLibrary::ProjectVectorOnToVector(FVector V, FVector Target) {
    if (Target.SizeSquared() > UE_SMALL_NUMBER) return V.ProjectOnTo(Target);
    ReportMathError("Divide by zero: ProjectVectorOnToVector with zero Target vector");
    return FVector::ZeroVector;
}
FVector UKismetMathLibrary::ProjectVectorOnToPlane(FVector V, FVector PlaneNormal) { return FVector::VectorPlaneProject(V, PlaneNormal); }
FVector UKismetMathLibrary::FindClosestPointOnSegment(FVector Point, FVector SegmentStart, FVector SegmentEnd) {
    return ClosestPointOnSegment(Point, SegmentStart, SegmentEnd);
}
FVector UKismetMathLibrary::FindClosestPointOnLine(FVector Point, FVector LineOrigin, FVector LineDirection) {
    const FVector SafeDir = LineDirection.GetSafeNormal();
    return LineOrigin + SafeDir * ((Point - LineOrigin) | SafeDir);
}
float UKismetMathLibrary::GetPointDistanceToSegment(FVector Point, FVector SegmentStart, FVector SegmentEnd) {
    return static_cast<float>((Point - ClosestPointOnSegment(Point, SegmentStart, SegmentEnd)).Size());
}
float UKismetMathLibrary::GetPointDistanceToLine(FVector Point, FVector LineOrigin, FVector LineDirection) {
    const FVector ClosestPoint = LineOrigin + LineDirection * (((Point - LineOrigin) | LineDirection) / (LineDirection | LineDirection));
    return static_cast<float>((Point - ClosestPoint).Size());
}
FVector UKismetMathLibrary::RandomUnitVector() { return RandomUnitVectorFromGlobalStream(); }
FVector UKismetMathLibrary::RandomPointInBoundingBox(const FVector Center, const FVector HalfSize) {
    const FVector Min = Center - HalfSize;
    const FVector Max = Center + HalfSize;
    return FVector(FMath::FRandRange(Min.X, Max.X), FMath::FRandRange(Min.Y, Max.Y), FMath::FRandRange(Min.Z, Max.Z));
}
// The cone sampling of FRandomStream::VRandCone on the global random numbers (UE's FMath::VRandCone).
FVector UKismetMathLibrary::RandomUnitVectorInConeInDegrees(FVector ConeDir, float ConeHalfAngleInDegrees) {
    const double ConeHalfAngleRad = FMath::DegreesToRadians(static_cast<double>(ConeHalfAngleInDegrees));
    if (ConeHalfAngleRad <= 0.0) return ConeDir.GetSafeNormal();
    const double Theta = 2.0 * UE_PI * FMath::FRand();
    const double Phi = FMath::Fmod(std::acos(2.0 * FMath::FRand() - 1.0), ConeHalfAngleRad);
    const FMatrix DirMat = FRotationMatrix(ConeDir.Rotation());
    const FVector Result = ConeDir.RotateAngleAxis(Phi * 180.0 / UE_PI, DirMat.GetScaledAxis(1));
    return Result.RotateAngleAxis(Theta * 180.0 / UE_PI, DirMat.GetScaledAxis(0)).GetSafeNormal();
}
FVector UKismetMathLibrary::Conv_IntToVector(int32 InInt) { return FVector(static_cast<double>(InInt)); }
FVector UKismetMathLibrary::Conv_DoubleToVector(double InDouble) { return FVector(InDouble); }

// --- Vector4 and matrices ---

FVector4 UKismetMathLibrary::MakeVector4(double X, double Y, double Z, double W) { return FVector4(X, Y, Z, W); }
void UKismetMathLibrary::BreakVector4(const FVector4& InVec, double& X, double& Y, double& Z, double& W) {
    X = InVec.X;
    Y = InVec.Y;
    Z = InVec.Z;
    W = InVec.W;
}
FVector UKismetMathLibrary::Conv_Vector4ToVector(const FVector4& InVector4) { return InVector4.XYZ(); }
FVector4 UKismetMathLibrary::TransformVector4(const FMatrix& Matrix, const FVector4& Vec4) { return Matrix.TransformFVector4(Vec4); }
FVector4 UKismetMathLibrary::Matrix_TransformPosition(const FMatrix& M, FVector V) { return M.TransformFVector4(FVector4(V, 1.0)); }

// --- Rotator ---

FRotator UKismetMathLibrary::MakeRotator(float Roll, float Pitch, float Yaw) { return FRotator(Pitch, Yaw, Roll); }
FRotator UKismetMathLibrary::MakeRotFromX(const FVector& X) { return FRotationMatrix::MakeFromX(X).Rotator(); }
FRotator UKismetMathLibrary::MakeRotFromZ(const FVector& Z) { return FRotationMatrix::MakeFromZ(Z).Rotator(); }
FRotator UKismetMathLibrary::MakeRotFromXY(const FVector& X, const FVector& Y) { return FRotationMatrix::MakeFromXY(X, Y).Rotator(); }
FRotator UKismetMathLibrary::MakeRotFromXZ(const FVector& X, const FVector& Z) { return FRotationMatrix::MakeFromXZ(X, Z).Rotator(); }
FRotator UKismetMathLibrary::MakeRotFromZX(const FVector& Z, const FVector& X) { return FRotationMatrix::MakeFromZX(Z, X).Rotator(); }
FRotator UKismetMathLibrary::FindLookAtRotation(const FVector& Start, const FVector& Target) { return MakeRotFromX(Target - Start); }
void UKismetMathLibrary::BreakRotator(FRotator InRot, float& Roll, float& Pitch, float& Yaw) {
    Roll = static_cast<float>(InRot.Roll);
    Pitch = static_cast<float>(InRot.Pitch);
    Yaw = static_cast<float>(InRot.Yaw);
}
bool UKismetMathLibrary::EqualEqual_RotatorRotator(FRotator A, FRotator B, float ErrorTolerance) { return A.Equals(B, ErrorTolerance); }
bool UKismetMathLibrary::NotEqual_RotatorRotator(FRotator A, FRotator B, float ErrorTolerance) { return !A.Equals(B, ErrorTolerance); }
FRotator UKismetMathLibrary::Multiply_RotatorFloat(FRotator A, float B) { return A * static_cast<double>(B); }
FRotator UKismetMathLibrary::ComposeRotators(FRotator A, FRotator B) { return FRotator(FQuat(B) * FQuat(A)); }
FRotator UKismetMathLibrary::NegateRotator(FRotator A) { return A.GetInverse(); }
FVector UKismetMathLibrary::GetForwardVector(FRotator InRot) { return InRot.Vector(); }
FVector UKismetMathLibrary::GetRightVector(FRotator InRot) { return FRotationMatrix(InRot).GetScaledAxis(1); }
FVector UKismetMathLibrary::GetUpVector(FRotator InRot) { return FRotationMatrix(InRot).GetScaledAxis(2); }
FVector UKismetMathLibrary::Conv_RotatorToVector(FRotator InRot) { return InRot.Vector(); }
FTransform UKismetMathLibrary::Conv_RotatorToTransform(const FRotator& InRotator) { return FTransform(InRotator); }
FRotator UKismetMathLibrary::RandomRotator(bool bRoll) {
    FRotator Result;
    Result.Yaw = FMath::FRand() * 360.0;
    Result.Pitch = FMath::FRand() * 360.0;
    Result.Roll = bRoll ? FMath::FRand() * 360.0 : 0.0;
    return Result;
}
FRotator UKismetMathLibrary::RLerp(FRotator A, FRotator B, float Alpha, bool bShortestPath) {
    if (bShortestPath) return FQuat::Slerp(FQuat(A), FQuat(B), static_cast<double>(Alpha)).Rotator();
    return A + (B - A) * static_cast<double>(Alpha);
}
FRotator UKismetMathLibrary::NormalizedDeltaRotator(FRotator A, FRotator B) {
    FRotator Delta = A - B;
    Delta.Normalize();
    return Delta;
}
FRotator UKismetMathLibrary::RInterpTo(FRotator Current, FRotator Target, float DeltaTime, float InterpSpeed) {
    if (DeltaTime == 0.f || Current == Target) return Current;
    if (InterpSpeed <= 0.f) return Target;
    const float DeltaInterpSpeed = InterpSpeed * DeltaTime;
    const FRotator Delta = (Target - Current).GetNormalized();
    if (Delta.IsNearlyZero()) return Target;
    return (Current + Delta * static_cast<double>(FMath::Clamp(DeltaInterpSpeed, 0.f, 1.f))).GetNormalized();
}
FRotator UKismetMathLibrary::RInterpTo_Constant(FRotator Current, FRotator Target, float DeltaTime, float InterpSpeed) {
    if (DeltaTime == 0.f || Current == Target) return Current;
    if (InterpSpeed <= 0.f) return Target;
    const double DeltaInterpSpeed = static_cast<double>(InterpSpeed) * DeltaTime;
    const FRotator DeltaMove = (Target - Current).GetNormalized();
    FRotator Result = Current;
    Result.Pitch += FMath::Clamp(DeltaMove.Pitch, -DeltaInterpSpeed, DeltaInterpSpeed);
    Result.Yaw += FMath::Clamp(DeltaMove.Yaw, -DeltaInterpSpeed, DeltaInterpSpeed);
    Result.Roll += FMath::Clamp(DeltaMove.Roll, -DeltaInterpSpeed, DeltaInterpSpeed);
    return Result.GetNormalized();
}
FVector UKismetMathLibrary::Quat_RotateVector(const FQuat& Q, const FVector& V) { return Q.RotateVector(V); }

// --- Color ---

FLinearColor UKismetMathLibrary::LinearColor_Transparent() { return FLinearColor(0.f, 0.f, 0.f, 0.f); }
FLinearColor UKismetMathLibrary::MakeColor(float R, float G, float B, float A) { return FLinearColor(R, G, B, A); }
void UKismetMathLibrary::BreakColor(FLinearColor InColor, float& R, float& G, float& B, float& A) {
    R = InColor.R;
    G = InColor.G;
    B = InColor.B;
    A = InColor.A;
}
FLinearColor UKismetMathLibrary::Conv_DoubleToLinearColor(double InDouble) {
    const float Value = static_cast<float>(InDouble);
    return FLinearColor(Value, Value, Value);
}
FLinearColor UKismetMathLibrary::HSVToRGB(float H, float S, float V, float A) { return FLinearColor(H, S, V, A).HSVToLinearRGB(); }
FLinearColor UKismetMathLibrary::HSVToRGBLinear(FLinearColor HSV) { return HSV.HSVToLinearRGB(); }
void UKismetMathLibrary::RGBToHSV(FLinearColor InColor, float& H, float& S, float& V, float& A) {
    const FLinearColor HSV = InColor.LinearRGBToHSV();
    H = HSV.R;
    S = HSV.G;
    V = HSV.B;
    A = HSV.A;
}
FVector UKismetMathLibrary::Conv_LinearColorToVector(FLinearColor InLinearColor) { return FVector(InLinearColor.R, InLinearColor.G, InLinearColor.B); }
FColor UKismetMathLibrary::Conv_LinearColorToColor(FLinearColor InLinearColor, bool InUseSRGB) { return InLinearColor.ToFColor(InUseSRGB); }
FLinearColor UKismetMathLibrary::LinearColor_ToNewOpacity(FLinearColor InColor, float InOpacity) { return InColor.CopyWithNewOpacity(InOpacity); }
FLinearColor UKismetMathLibrary::LinearColorLerp(FLinearColor A, FLinearColor B, float Alpha) { return A + (B - A) * Alpha; }
FLinearColor UKismetMathLibrary::LinearColorLerpUsingHSV(FLinearColor A, FLinearColor B, float Alpha) { return FLinearColor::LerpUsingHSV(A, B, Alpha); }
bool UKismetMathLibrary::EqualEqual_LinearColorLinearColor(FLinearColor A, FLinearColor B) { return A == B; }
bool UKismetMathLibrary::NotEqual_LinearColorLinearColor(FLinearColor A, FLinearColor B) { return !(A == B); }
FLinearColor UKismetMathLibrary::Multiply_LinearColorLinearColor(FLinearColor A, FLinearColor B) { return A * B; }
FLinearColor UKismetMathLibrary::Conv_ColorToLinearColor(FColor InColor) { return FLinearColor(InColor); }

// --- Plane and box ---

FPlane UKismetMathLibrary::MakePlaneFromPointAndNormal(FVector Point, FVector Normal) { return FPlane(Point, Normal.GetSafeNormal()); }
FBox UKismetMathLibrary::MakeBox(FVector Min, FVector Max) { return FBox(Min, Max); }
void UKismetMathLibrary::BreakBoxSphereBounds(const FBoxSphereBounds& InBoxSphereBounds, FVector& Origin, FVector& BoxExtent, float& SphereRadius) {
    Origin = InBoxSphereBounds.Origin;
    BoxExtent = InBoxSphereBounds.BoxExtent;
    SphereRadius = static_cast<float>(InBoxSphereBounds.SphereRadius);
}
bool UKismetMathLibrary::IsPointInBox(FVector Point, FVector BoxOrigin, FVector BoxExtent) {
    return FBox(BoxOrigin - BoxExtent, BoxOrigin + BoxExtent).IsInsideOrOn(Point);
}
bool UKismetMathLibrary::IsPointInBoxWithTransform(FVector Point, const FTransform& BoxWorldTransform, FVector BoxExtent) {
    return FBox(-BoxExtent, BoxExtent).IsInsideOrOn(BoxWorldTransform.InverseTransformPosition(Point));
}
bool UKismetMathLibrary::LinePlaneIntersection(const FVector& LineStart, const FVector& LineEnd, const FPlane& APlane, float& T, FVector& Intersection) {
    const FVector RayDir = LineEnd - LineStart;
    const FVector Normal = APlane.GetNormal();
    if ((RayDir | Normal) == 0.0) {
        T = -1.f;
        Intersection = FVector::ZeroVector;
        return false;
    }
    T = static_cast<float>((APlane.W - (LineStart | Normal)) / (RayDir | Normal));
    if (T < 0.f || T > 1.f) {
        Intersection = FVector::ZeroVector;
        return false;
    }
    Intersection = LineStart + RayDir * static_cast<double>(T);
    return true;
}
bool UKismetMathLibrary::LinePlaneIntersection_OriginNormal(const FVector& LineStart, const FVector& LineEnd, FVector PlaneOrigin, FVector PlaneNormal,
                                                            float& T, FVector& Intersection) {
    return LinePlaneIntersection(LineStart, LineEnd, FPlane(PlaneOrigin, PlaneNormal), T, Intersection);
}

// --- Date and time ---

FDateTime UKismetMathLibrary::MakeDateTime(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute, int32 Second, int32 Millisecond) {
    if (FDateTime::Validate(Year, Month, Day, Hour, Minute, Second, Millisecond))
        return FDateTime(Year, Month, Day, Hour, Minute, Second, Millisecond);
    ReportMathError("DateTime in bad format");
    return FDateTime();
}
void UKismetMathLibrary::BreakDateTime(FDateTime InDateTime, int32& Year, int32& Month, int32& Day, int32& Hour, int32& Minute, int32& Second,
                                       int32& Millisecond) {
    InDateTime.GetDate(Year, Month, Day);
    Hour = InDateTime.GetHour();
    Minute = InDateTime.GetMinute();
    Second = InDateTime.GetSecond();
    Millisecond = InDateTime.GetMillisecond();
}
FDateTime UKismetMathLibrary::Add_DateTimeTimespan(FDateTime A, FTimespan B) { return A + B; }
FDateTime UKismetMathLibrary::Subtract_DateTimeTimespan(FDateTime A, FTimespan B) { return A - B; }
FTimespan UKismetMathLibrary::Subtract_DateTimeDateTime(FDateTime A, FDateTime B) { return A - B; }
bool UKismetMathLibrary::EqualEqual_DateTimeDateTime(FDateTime A, FDateTime B) { return A == B; }
bool UKismetMathLibrary::NotEqual_DateTimeDateTime(FDateTime A, FDateTime B) { return A != B; }
bool UKismetMathLibrary::Greater_DateTimeDateTime(FDateTime A, FDateTime B) { return A > B; }
bool UKismetMathLibrary::GreaterEqual_DateTimeDateTime(FDateTime A, FDateTime B) { return A >= B; }
bool UKismetMathLibrary::Less_DateTimeDateTime(FDateTime A, FDateTime B) { return A < B; }
bool UKismetMathLibrary::LessEqual_DateTimeDateTime(FDateTime A, FDateTime B) { return A <= B; }
int32 UKismetMathLibrary::GetMillisecond(FDateTime A) { return A.GetMillisecond(); }
FDateTime UKismetMathLibrary::DateTimeMaxValue() { return FDateTime::MaxValue(); }
FDateTime UKismetMathLibrary::DateTimeMinValue() { return FDateTime::MinValue(); }
FDateTime UKismetMathLibrary::Now() { return FDateTime::Now(); }
FDateTime UKismetMathLibrary::UtcNow() { return FDateTime::UtcNow(); }
bool UKismetMathLibrary::DateTimeFromString(FString DateTimeString, FDateTime& Result) { return FDateTime::Parse(DateTimeString, Result); }
double UKismetMathLibrary::ToUnixTimestampDouble(const FDateTime& Time) { return Time.ToUnixTimestampDecimal(); }
FTimespan UKismetMathLibrary::TimespanZeroValue() { return FTimespan::Zero(); }
FTimespan UKismetMathLibrary::MakeTimespan(int32 Days, int32 Hours, int32 Minutes, int32 Seconds, int32 Milliseconds) {
    return FTimespan(Days, Hours, Minutes, Seconds, Milliseconds * 1000000);
}
void UKismetMathLibrary::BreakTimespan(FTimespan InTimespan, int32& Days, int32& Hours, int32& Minutes, int32& Seconds, int32& Milliseconds) {
    Days = InTimespan.GetDays();
    Hours = InTimespan.GetHours();
    Minutes = InTimespan.GetMinutes();
    Seconds = InTimespan.GetSeconds();
    Milliseconds = InTimespan.GetFractionMilli();
}
FTimespan UKismetMathLibrary::Subtract_TimespanTimespan(FTimespan A, FTimespan B) { return A - B; }
bool UKismetMathLibrary::EqualEqual_TimespanTimespan(FTimespan A, FTimespan B) { return A == B; }
bool UKismetMathLibrary::NotEqual_TimespanTimespan(FTimespan A, FTimespan B) { return A != B; }
bool UKismetMathLibrary::Greater_TimespanTimespan(FTimespan A, FTimespan B) { return A > B; }
bool UKismetMathLibrary::GreaterEqual_TimespanTimespan(FTimespan A, FTimespan B) { return A >= B; }
bool UKismetMathLibrary::Less_TimespanTimespan(FTimespan A, FTimespan B) { return A < B; }
int32 UKismetMathLibrary::GetDays(FTimespan A) { return A.GetDays(); }
float UKismetMathLibrary::GetTotalSeconds(FTimespan A) { return static_cast<float>(A.GetTotalSeconds()); }
FTimespan UKismetMathLibrary::FromHours(float Hours) { return FTimespan::FromHours(Hours); }
FTimespan UKismetMathLibrary::FromMilliseconds(float Milliseconds) { return FTimespan::FromMilliseconds(Milliseconds); }
FTimespan UKismetMathLibrary::FromSeconds(float Seconds) { return FTimespan::FromSeconds(Seconds); }

// --- Conversions ---

double UKismetMathLibrary::Conv_ByteToDouble(uint8 InByte) { return InByte; }
double UKismetMathLibrary::Conv_IntToDouble(int32 InInt) { return InInt; }
int64 UKismetMathLibrary::Conv_IntToInt64(int32 InInt) { return InInt; }
uint8 UKismetMathLibrary::Conv_IntToByte(int32 InInt) { return static_cast<uint8>(InInt); }
int32 UKismetMathLibrary::Conv_Int64ToInt(int64 InInt) { return static_cast<int32>(InInt); }
int64 UKismetMathLibrary::Conv_DoubleToInt64(double InDouble) { return FMath::TruncToInt64(InDouble); }
bool UKismetMathLibrary::Conv_IntToBool(int32 InInt) { return InInt != 0; }
int32 UKismetMathLibrary::Conv_BoolToInt(bool InBool) { return InBool ? 1 : 0; }
double UKismetMathLibrary::Conv_BoolToDouble(bool InBool) { return InBool ? 1.0 : 0.0; }
uint8 UKismetMathLibrary::Conv_BoolToByte(bool InBool) { return InBool ? 1 : 0; }
int32 UKismetMathLibrary::Conv_ByteToInt(uint8 InByte) { return InByte; }
int64 UKismetMathLibrary::Conv_ByteToInt64(uint8 InByte) { return InByte; }

// --- Random streams ---

FRandomStream UKismetMathLibrary::MakeRandomStream(int32 InitialSeed) { return FRandomStream(InitialSeed); }
int32 UKismetMathLibrary::RandomIntegerFromStream(const FRandomStream& Stream, int32 Max) { return Stream.RandHelper(Max); }
int32 UKismetMathLibrary::RandomIntegerInRangeFromStream(const FRandomStream& Stream, int32 Min, int32 Max) { return Stream.RandRange(Min, Max); }
bool UKismetMathLibrary::RandomBoolFromStream(const FRandomStream& Stream) { return Stream.RandRange(0, 1) == 1; }
float UKismetMathLibrary::RandomFloatFromStream(const FRandomStream& Stream) { return Stream.FRand(); }
float UKismetMathLibrary::RandomFloatInRangeFromStream(const FRandomStream& Stream, float Min, float Max) { return Stream.FRandRange(Min, Max); }
FVector UKismetMathLibrary::RandomUnitVectorFromStream(const FRandomStream& Stream) { return Stream.VRand(); }
void UKismetMathLibrary::SetRandomStreamSeed(FRandomStream& Stream, int32 NewSeed) { Stream.Initialize(NewSeed); }

// --- Selection and identity ---

FString UKismetMathLibrary::SelectString(const FString& A, const FString& B, bool bPickA) { return bPickA ? A : B; }
int32 UKismetMathLibrary::SelectInt(int32 A, int32 B, bool bPickA) { return bPickA ? A : B; }
double UKismetMathLibrary::SelectFloat(double A, double B, bool bPickA) { return bPickA ? A : B; }
FVector UKismetMathLibrary::SelectVector(FVector A, FVector B, bool bPickA) { return bPickA ? A : B; }
FRotator UKismetMathLibrary::SelectRotator(FRotator A, FRotator B, bool bPickA) { return bPickA ? A : B; }
FLinearColor UKismetMathLibrary::SelectColor(FLinearColor A, FLinearColor B, bool bPickA) { return bPickA ? A : B; }
FTransform UKismetMathLibrary::SelectTransform(const FTransform& A, const FTransform& B, bool bPickA) { return bPickA ? A : B; }
UObject* UKismetMathLibrary::SelectObject(UObject* A, UObject* B, bool bSelectA) { return bSelectA ? A : B; }
bool UKismetMathLibrary::EqualEqual_ObjectObject(UObject* A, UObject* B) { return A == B; }
bool UKismetMathLibrary::NotEqual_ObjectObject(UObject* A, UObject* B) { return A != B; }
bool UKismetMathLibrary::EqualEqual_ClassClass(UClass* A, UClass* B) { return A == B; }
bool UKismetMathLibrary::NotEqual_ClassClass(UClass* A, UClass* B) { return A != B; }
bool UKismetMathLibrary::ClassIsChildOf(TSubclassOf<UObject> TestClass, TSubclassOf<UObject> ParentClass) {
    return ParentClass.Get() != nullptr && TestClass.Get() != nullptr && TestClass->IsChildOf(ParentClass);
}
bool UKismetMathLibrary::EqualEqual_NameName(FName A, FName B) { return A == B; }
bool UKismetMathLibrary::NotEqual_NameName(FName A, FName B) { return A != B; }

// --- Transform ---

FTransform UKismetMathLibrary::MakeTransform(FVector Location, FRotator Rotation, FVector Scale) { return FTransform(Rotation, Location, Scale); }
void UKismetMathLibrary::BreakTransform(const FTransform& InTransform, FVector& Location, FRotator& Rotation, FVector& Scale) {
    Location = InTransform.GetLocation();
    Rotation = InTransform.Rotator();
    Scale = InTransform.GetScale3D();
}
bool UKismetMathLibrary::EqualEqual_TransformTransform(const FTransform& A, const FTransform& B) { return A.Equals(B); }
FTransform UKismetMathLibrary::ComposeTransforms(const FTransform& A, const FTransform& B) { return A * B; }
FVector UKismetMathLibrary::TransformLocation(const FTransform& T, FVector Location) { return T.TransformPosition(Location); }
FVector UKismetMathLibrary::TransformDirection(const FTransform& T, FVector Direction) { return T.TransformVectorNoScale(Direction); }
FVector UKismetMathLibrary::InverseTransformLocation(const FTransform& T, FVector Location) { return T.InverseTransformPosition(Location); }
FVector UKismetMathLibrary::InverseTransformDirection(const FTransform& T, FVector Direction) { return T.InverseTransformVectorNoScale(Direction); }
FRotator UKismetMathLibrary::InverseTransformRotation(const FTransform& T, FRotator Rotation) {
    return T.InverseTransformRotation(Rotation.Quaternion()).Rotator();
}
FTransform UKismetMathLibrary::MakeRelativeTransform(const FTransform& A, const FTransform& RelativeTo) { return A.GetRelativeTransform(RelativeTo); }
FTransform UKismetMathLibrary::InvertTransform(const FTransform& T) { return T.Inverse(); }
FTransform UKismetMathLibrary::TLerp(const FTransform& A, const FTransform& B, float Alpha, TEnumAsByte<ELerpInterpolationMode::Type> InterpMode) {
    FTransform NA = A;
    FTransform NB = B;
    NA.NormalizeRotation();
    NB.NormalizeRotation();
    const double Blend = Alpha;
    if (InterpMode == ELerpInterpolationMode::EulerInterp) {
        FTransform Result;
        Result.SetTranslation(NA.GetTranslation() + (NB.GetTranslation() - NA.GetTranslation()) * Blend);
        Result.SetScale3D(NA.GetScale3D() + (NB.GetScale3D() - NA.GetScale3D()) * Blend);
        Result.SetRotation(FQuat(RLerp(NA.Rotator(), NB.Rotator(), Alpha, false)));
        return Result;
    }
    if (InterpMode == ELerpInterpolationMode::DualQuatInterp) {
        const FDualQuat Blended = FDualQuat(NA) * (1.0 - Blend) + FDualQuat(NB) * Blend;
        return Blended.Normalized().AsFTransform(NA.GetScale3D() + (NB.GetScale3D() - NA.GetScale3D()) * Blend);
    }
    FTransform Result;
    Result.Blend(NA, NB, Alpha);
    return Result;
}

}
