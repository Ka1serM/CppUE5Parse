#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"
#include "ue5parse/runtime/CoreUObject/Templates/SubclassOf.h"
#include "ue5parse/runtime/Engine/Kismet/BlueprintFunctionLibrary.h"

#include "KismetMathLibrary.generated.h"

namespace ue5parse::runtime {

namespace EEasingFunc {
enum Type : int {
    Linear,
    Step,
    SinusoidalIn,
    SinusoidalOut,
    SinusoidalInOut,
    EaseIn,
    EaseOut,
    EaseInOut,
    ExpoIn,
    ExpoOut,
    ExpoInOut,
    CircularIn,
    CircularOut,
    CircularInOut,
};
}

namespace ELerpInterpolationMode {
enum Type : int {
    QuatInterp,
    EulerInterp,
    DualQuatInterp,
};
}

// UKismetMathLibrary: the math functions of Blueprints (UE's Kismet/KismetMathLibrary.h). Only the functions the
// cooked game calls are declared.
UCLASS()
class UKismetMathLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure)
    static bool RandomBool();
    UFUNCTION(BlueprintPure)
    static bool RandomBoolWithWeight(float Weight);
    UFUNCTION(BlueprintPure)
    static bool RandomBoolWithWeightFromStream(const FRandomStream& RandomStream, float Weight);
    UFUNCTION(BlueprintPure)
    static bool Not_PreBool(bool A);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_BoolBool(bool A, bool B);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_BoolBool(bool A, bool B);
    UFUNCTION(BlueprintPure)
    static bool BooleanAND(bool A, bool B);
    UFUNCTION(BlueprintPure)
    static bool BooleanNAND(bool A, bool B);
    UFUNCTION(BlueprintPure)
    static bool BooleanOR(bool A, bool B);
    UFUNCTION(BlueprintPure)
    static bool BooleanXOR(bool A, bool B);
    UFUNCTION(BlueprintPure)
    static bool BooleanNOR(bool A, bool B);
    UFUNCTION(BlueprintPure)
    static bool Less_ByteByte(uint8 A, uint8 B);
    UFUNCTION(BlueprintPure)
    static bool Greater_ByteByte(uint8 A, uint8 B);
    UFUNCTION(BlueprintPure)
    static bool LessEqual_ByteByte(uint8 A, uint8 B);
    UFUNCTION(BlueprintPure)
    static bool GreaterEqual_ByteByte(uint8 A, uint8 B);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_ByteByte(uint8 A, uint8 B);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_ByteByte(uint8 A, uint8 B);
    UFUNCTION(BlueprintPure)
    static int32 Multiply_IntInt(int32 A, int32 B);
    UFUNCTION(BlueprintPure)
    static int32 Divide_IntInt(int32 A, int32 B = 1);
    UFUNCTION(BlueprintPure)
    static int32 Percent_IntInt(int32 A, int32 B = 1);
    UFUNCTION(BlueprintPure)
    static int32 Add_IntInt(int32 A, int32 B = 1);
    UFUNCTION(BlueprintPure)
    static int32 Subtract_IntInt(int32 A, int32 B = 1);
    UFUNCTION(BlueprintPure)
    static bool Less_IntInt(int32 A, int32 B);
    UFUNCTION(BlueprintPure)
    static bool Greater_IntInt(int32 A, int32 B);
    UFUNCTION(BlueprintPure)
    static bool LessEqual_IntInt(int32 A, int32 B);
    UFUNCTION(BlueprintPure)
    static bool GreaterEqual_IntInt(int32 A, int32 B);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_IntInt(int32 A, int32 B);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_IntInt(int32 A, int32 B);
    UFUNCTION(BlueprintPure)
    static bool InRange_IntInt(int32 Value, int32 Min, int32 Max, bool InclusiveMin = true, bool InclusiveMax = true);
    UFUNCTION(BlueprintPure)
    static int32 And_IntInt(int32 A, int32 B);
    UFUNCTION(BlueprintPure)
    static int32 SignOfInteger(int32 A);
    UFUNCTION(BlueprintPure)
    static int32 RandomInteger(int32 Max);
    UFUNCTION(BlueprintPure)
    static int32 RandomIntegerInRange(int32 Min, int32 Max);
    UFUNCTION(BlueprintPure)
    static int32 Min(int32 A, int32 B);
    UFUNCTION(BlueprintPure)
    static int32 Max(int32 A, int32 B);
    UFUNCTION(BlueprintPure)
    static int32 Clamp(int32 Value, int32 Min, int32 Max);
    UFUNCTION(BlueprintPure)
    static int32 Abs_Int(int32 A);
    UFUNCTION(BlueprintPure)
    static bool Greater_Int64Int64(int64 A, int64 B);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_Int64Int64(int64 A, int64 B);
    UFUNCTION(BlueprintPure)
    static double MultiplyMultiply_FloatFloat(double Base, double Exp);
    UFUNCTION(BlueprintPure)
    static double Multiply_IntFloat(int32 A, double B);
    UFUNCTION(BlueprintPure)
    static double Percent_FloatFloat(double A, double B = 1.f);
    UFUNCTION(BlueprintPure)
    static double Fraction(double A);
    UFUNCTION(BlueprintPure)
    static double Add_DoubleDouble(double A, double B = 1.0);
    UFUNCTION(BlueprintPure)
    static double Subtract_DoubleDouble(double A, double B = 1.0);
    UFUNCTION(BlueprintPure)
    static double Multiply_DoubleDouble(double A, double B);
    UFUNCTION(BlueprintPure)
    static double Divide_DoubleDouble(double A, double B = 1.0);
    UFUNCTION(BlueprintPure)
    static bool Less_DoubleDouble(double A, double B);
    UFUNCTION(BlueprintPure)
    static bool Greater_DoubleDouble(double A, double B);
    UFUNCTION(BlueprintPure)
    static bool LessEqual_DoubleDouble(double A, double B);
    UFUNCTION(BlueprintPure)
    static bool GreaterEqual_DoubleDouble(double A, double B);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_DoubleDouble(double A, double B);
    UFUNCTION(BlueprintPure)
    static bool NearlyEqual_FloatFloat(double A, double B, double ErrorTolerance = 1.e-6);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_DoubleDouble(double A, double B);
    UFUNCTION(BlueprintPure)
    static bool InRange_FloatFloat(double Value, double Min, double Max, bool InclusiveMin = true, bool InclusiveMax = true);
    UFUNCTION(BlueprintPure)
    static double GridSnap_Float(double Location, double GridSize);
    UFUNCTION(BlueprintPure)
    static double Abs(double A);
    UFUNCTION(BlueprintPure)
    static double Sin(double A);
    UFUNCTION(BlueprintPure)
    static double Exp(double A);
    UFUNCTION(BlueprintPure)
    static double Sqrt(double A);
    UFUNCTION(BlueprintPure)
    static double Square(double A);
    UFUNCTION(BlueprintPure)
    static double RandomFloat();
    UFUNCTION(BlueprintPure)
    static double RandomFloatInRange(double Min, double Max);
    UFUNCTION(BlueprintPure)
    static double GetPI();
    UFUNCTION(BlueprintPure)
    static double DegreesToRadians(double A);
    UFUNCTION(BlueprintPure)
    static double RadiansToDegrees(double A);
    UFUNCTION(BlueprintPure)
    static double DegSin(double A);
    UFUNCTION(BlueprintPure)
    static double DegAsin(double A);
    UFUNCTION(BlueprintPure)
    static double DegCos(double A);
    UFUNCTION(BlueprintPure)
    static double DegAcos(double A);
    UFUNCTION(BlueprintPure)
    static double DegTan(double A);
    UFUNCTION(BlueprintPure)
    static double DegAtan(double A);
    UFUNCTION(BlueprintPure)
    static double DegAtan2(double Y, double X);
    UFUNCTION(BlueprintPure)
    static double ClampAngle(double AngleDegrees, double MinAngleDegrees, double MaxAngleDegrees);
    UFUNCTION(BlueprintPure)
    static double FMin(double A, double B);
    UFUNCTION(BlueprintPure)
    static double FMax(double A, double B);
    UFUNCTION(BlueprintPure)
    static double FClamp(double Value, double Min, double Max);
    UFUNCTION(BlueprintPure)
    static double FWrap(double Value, double Min, double Max);
    UFUNCTION(BlueprintPure)
    static double SafeDivide(double A, double B);
    UFUNCTION(BlueprintPure)
    static void MaxOfIntArray(const TArray<int32>& IntArray, int32& IndexOfMaxValue, int32& MaxValue);
    UFUNCTION(BlueprintPure)
    static void MaxOfFloatArray(const TArray<float>& FloatArray, int32& IndexOfMaxValue, float& MaxValue);
    UFUNCTION(BlueprintPure)
    static double Lerp(double A, double B, double Alpha);
    UFUNCTION(BlueprintPure)
    static double Ease(double A, double B, double Alpha, TEnumAsByte<EEasingFunc::Type> EasingFunc, double BlendExp = 2, int32 Steps = 2);
    UFUNCTION(BlueprintPure)
    static int32 Round(double A);
    UFUNCTION(BlueprintPure)
    static int32 FFloor(double A);
    UFUNCTION(BlueprintPure)
    static int32 FTrunc(double A);
    UFUNCTION(BlueprintPure)
    static int32 FCeil(double A);
    UFUNCTION(BlueprintPure)
    static int64 FTrunc64(double A);
    UFUNCTION(BlueprintPure)
    static int32 FMod(double Dividend, double Divisor, double& Remainder);
    UFUNCTION(BlueprintPure)
    static double SignOfFloat(double A);
    UFUNCTION(BlueprintPure)
    static double NormalizeToRange(double Value, double RangeMin, double RangeMax);
    UFUNCTION(BlueprintPure)
    static double MapRangeUnclamped(double Value, double InRangeA, double InRangeB, double OutRangeA, double OutRangeB);
    UFUNCTION(BlueprintPure)
    static double MapRangeClamped(double Value, double InRangeA, double InRangeB, double OutRangeA, double OutRangeB);
    UFUNCTION(BlueprintPure)
    static double MultiplyByPi(double Value);
    UFUNCTION(BlueprintPure)
    static bool Equal_IntPointIntPoint(FIntPoint A, FIntPoint B);
    UFUNCTION(BlueprintPure)
    static FVector2D MakeVector2D(double X, double Y);
    UFUNCTION(BlueprintPure)
    static void BreakVector2D(FVector2D InVec, double& X, double& Y);
    UFUNCTION(BlueprintPure)
    static FVector Conv_Vector2DToVector(FVector2D InVector2D, float Z = 0);
    UFUNCTION(BlueprintPure)
    static FVector2D Add_Vector2DVector2D(FVector2D A, FVector2D B);
    UFUNCTION(BlueprintPure)
    static FVector2D Subtract_Vector2DVector2D(FVector2D A, FVector2D B);
    UFUNCTION(BlueprintPure)
    static FVector2D Multiply_Vector2DVector2D(FVector2D A, FVector2D B);
    UFUNCTION(BlueprintPure)
    static FVector2D Multiply_Vector2DFloat(FVector2D A, double B);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_Vector2DVector2D(FVector2D A, FVector2D B, float ErrorTolerance = 1.e-4f);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_Vector2DVector2D(FVector2D A, FVector2D B, float ErrorTolerance = 1.e-4f);
    UFUNCTION(BlueprintPure)
    static double Distance2D(FVector2D V1, FVector2D V2);
    UFUNCTION(BlueprintPure)
    static double DistanceSquared2D(FVector2D V1, FVector2D V2);
    UFUNCTION(BlueprintPure)
    static double DotProduct2D(FVector2D A, FVector2D B);
    UFUNCTION(BlueprintPure)
    static FVector2D GetRotated2D(FVector2D A, float AngleDeg);
    UFUNCTION(BlueprintPure)
    static bool IsNearlyZero2D(const FVector2D& A, float Tolerance = 1.e-4f);
    UFUNCTION(BlueprintPure)
    static FVector2D NormalSafe2D(FVector2D A, float Tolerance = 1.e-8f);
    UFUNCTION(BlueprintPure)
    static FVector2D Normal2D(FVector2D A);
    UFUNCTION(BlueprintPure)
    static double VSize2D(FVector2D A);
    UFUNCTION(BlueprintPure)
    static double VSize2DSquared(FVector2D A);
    UFUNCTION(BlueprintPure)
    static FVector Vector_Zero();
    UFUNCTION(BlueprintPure)
    static FVector Vector_Up();
    UFUNCTION(BlueprintPure)
    static FVector MakeVector(double X, double Y, double Z);
    UFUNCTION(BlueprintPure)
    static FVector CreateVectorFromYawPitch(float Yaw, float Pitch, float Length = 1.0f );
    UFUNCTION(BlueprintCallable)
    static void Vector_Set(UPARAM(ref) FVector& A, double X, double Y, double Z);
    UFUNCTION(BlueprintPure)
    static void BreakVector(FVector InVec, double& X, double& Y, double& Z);
    UFUNCTION(BlueprintPure)
    static FLinearColor Conv_VectorToLinearColor(FVector InVec);
    UFUNCTION(BlueprintPure)
    static FTransform Conv_VectorToTransform(FVector InLocation);
    UFUNCTION(BlueprintPure)
    static FVector2D Conv_VectorToVector2D(FVector InVector);
    UFUNCTION(BlueprintPure)
    static FRotator Conv_VectorToRotator(FVector InVec);
    UFUNCTION(BlueprintPure)
    static FRotator RotatorFromAxisAndAngle(FVector Axis, float Angle);
    UFUNCTION(BlueprintPure)
    static FQuat Conv_VectorToQuaternion(FVector InVec);
    UFUNCTION(BlueprintPure)
    static FVector Add_VectorVector(FVector A, FVector B);
    UFUNCTION(BlueprintPure)
    static FVector Add_VectorFloat(FVector A, double B);
    UFUNCTION(BlueprintPure)
    static FVector Subtract_VectorVector(FVector A, FVector B);
    UFUNCTION(BlueprintPure)
    static FVector Subtract_VectorFloat(FVector A, double B);
    UFUNCTION(BlueprintPure)
    static FVector Multiply_VectorVector(FVector A, FVector B);
    UFUNCTION(BlueprintPure)
    static FVector Multiply_VectorFloat(FVector A, double B);
    UFUNCTION(BlueprintPure)
    static FVector Multiply_VectorInt(FVector A, int32 B);
    UFUNCTION(BlueprintPure)
    static FVector Divide_VectorVector(FVector A, FVector B = FVector(1.f,1.f,1.f));
    UFUNCTION(BlueprintPure)
    static FVector Divide_VectorFloat(FVector A, double B = 1.f);
    UFUNCTION(BlueprintPure)
    static FVector Divide_VectorInt(FVector A, int32 B = 1);
    UFUNCTION(BlueprintPure)
    static FVector NegateVector(FVector A);
    UFUNCTION(BlueprintPure)
    static bool EqualExactly_VectorVector(FVector A, FVector B);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_VectorVector(FVector A, FVector B, float ErrorTolerance = 1.e-4f);
    UFUNCTION(BlueprintPure)
    static bool NotEqualExactly_VectorVector(FVector A, FVector B);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_VectorVector(FVector A, FVector B, float ErrorTolerance = 1.e-4f);
    UFUNCTION(BlueprintPure)
    static double Dot_VectorVector(FVector A, FVector B);
    UFUNCTION(BlueprintPure)
    static FVector Cross_VectorVector(FVector A, FVector B);
    UFUNCTION(BlueprintPure)
    static FVector GreaterGreater_VectorRotator(FVector A, FRotator B);
    UFUNCTION(BlueprintPure)
    static FVector RotateAngleAxis(FVector InVect, float AngleDeg, FVector Axis);
    UFUNCTION(BlueprintPure)
    static FVector LessLess_VectorRotator(FVector A, FRotator B);
    UFUNCTION(BlueprintPure)
    static FVector ClampVectorSize(FVector A, double Min, double Max);
    UFUNCTION(BlueprintPure)
    static FVector Vector_ClampSizeMax(FVector A, double Max);
    UFUNCTION(BlueprintPure)
    static FVector Vector_GetAbs(FVector A);
    UFUNCTION(BlueprintPure)
    static FVector Vector_ComponentMin(FVector A, FVector B);
    UFUNCTION(BlueprintPure)
    static FVector Vector_ComponentMax(FVector A, FVector B);
    UFUNCTION(BlueprintPure)
    static double Vector_HeadingAngle(FVector A);
    UFUNCTION(BlueprintPure)
    static double Vector_CosineAngle2D(FVector A, FVector B);
    UFUNCTION(BlueprintPure)
    static FVector GetDirectionUnitVector(FVector From, FVector To);
    UFUNCTION(BlueprintPure)
    static void GetYawPitchFromVector(FVector InVec, float& Yaw, float& Pitch);
    UFUNCTION(BlueprintPure)
    static FVector GetVectorArrayAverage(const TArray<FVector>& Vectors);
    UFUNCTION(BlueprintPure)
    static double Vector_Distance(FVector V1, FVector V2);
    UFUNCTION(BlueprintPure)
    static double Vector_DistanceSquared(FVector V1, FVector V2);
    UFUNCTION(BlueprintPure)
    static double Vector_Distance2D(FVector V1, FVector V2);
    UFUNCTION(BlueprintPure)
    static double Vector_Distance2DSquared(FVector V1, FVector V2);
    UFUNCTION(BlueprintPure)
    static double VSize(FVector A);
    UFUNCTION(BlueprintPure)
    static double VSizeSquared(FVector A);
    UFUNCTION(BlueprintPure)
    static double VSizeXY(FVector A);
    UFUNCTION(BlueprintPure)
    static double VSizeXYSquared(FVector A);
    UFUNCTION(BlueprintPure)
    static bool Vector_IsNearlyZero(const FVector& A, float Tolerance = 1.e-4f);
    UFUNCTION(BlueprintPure)
    static bool Vector_IsZero(const FVector& A);
    UFUNCTION(BlueprintPure)
    static bool Vector_IsNAN(const FVector& A);
    UFUNCTION(BlueprintPure)
    static FVector Normal(FVector A, float Tolerance = 1.e-4f);
    UFUNCTION(BlueprintPure)
    static FVector Vector_Normal2D(FVector A, float Tolerance = 1.e-4f);
    UFUNCTION(BlueprintPure)
    static FVector VLerp(FVector A, FVector B, float Alpha);
    UFUNCTION(BlueprintPure)
    static FVector VEase(FVector A, FVector B, float Alpha, TEnumAsByte<EEasingFunc::Type> EasingFunc, float BlendExp = 2, int32 Steps = 2);
    UFUNCTION(BlueprintPure)
    static FVector VInterpTo(FVector Current, FVector Target, float DeltaTime, float InterpSpeed);
    UFUNCTION(BlueprintPure)
    static FVector VInterpTo_Constant(FVector Current, FVector Target, float DeltaTime, float InterpSpeed);
    UFUNCTION(BlueprintPure)
    static FVector GetReflectionVector(FVector Direction, FVector SurfaceNormal);
    UFUNCTION(BlueprintPure)
    static FVector MirrorVectorByNormal(FVector InVect, FVector InNormal);
    UFUNCTION(BlueprintPure)
    static FVector Vector_ProjectOnToNormal(FVector V, FVector InNormal);
    UFUNCTION(BlueprintPure)
    static FVector ProjectVectorOnToVector(FVector V, FVector Target);
    UFUNCTION(BlueprintPure)
    static FVector ProjectVectorOnToPlane(FVector V, FVector PlaneNormal);
    UFUNCTION(BlueprintPure)
    static FVector FindClosestPointOnSegment(FVector Point, FVector SegmentStart, FVector SegmentEnd);
    UFUNCTION(BlueprintPure)
    static FVector FindClosestPointOnLine(FVector Point, FVector LineOrigin, FVector LineDirection);
    UFUNCTION(BlueprintPure)
    static float GetPointDistanceToSegment(FVector Point, FVector SegmentStart, FVector SegmentEnd);
    UFUNCTION(BlueprintPure)
    static float GetPointDistanceToLine(FVector Point, FVector LineOrigin, FVector LineDirection);
    UFUNCTION(BlueprintPure)
    static FVector RandomUnitVector();
    UFUNCTION(BlueprintPure)
    static FVector RandomPointInBoundingBox(const FVector Center, const FVector HalfSize);
    UFUNCTION(BlueprintPure)
    static FVector RandomUnitVectorInConeInDegrees(FVector ConeDir, float ConeHalfAngleInDegrees);
    UFUNCTION(BlueprintPure)
    static FVector4 MakeVector4(double X, double Y, double Z, double W);
    UFUNCTION(BlueprintPure)
    static void BreakVector4(const FVector4& InVec, double& X, double& Y, double& Z, double& W);
    UFUNCTION(BlueprintPure)
    static FVector Conv_Vector4ToVector(const FVector4& InVector4);
    UFUNCTION(BlueprintPure)
    static FVector4 TransformVector4(const FMatrix& Matrix, const FVector4& Vec4);
    UFUNCTION(BlueprintPure)
    static FRotator MakeRotator( float Roll, float Pitch, float Yaw);
    UFUNCTION(BlueprintPure)
    static FRotator MakeRotFromX(const FVector& X);
    UFUNCTION(BlueprintPure)
    static FRotator MakeRotFromZ(const FVector& Z);
    UFUNCTION(BlueprintPure)
    static FRotator MakeRotFromXY(const FVector& X, const FVector& Y);
    UFUNCTION(BlueprintPure)
    static FRotator MakeRotFromXZ(const FVector& X, const FVector& Z);
    UFUNCTION(BlueprintPure)
    static FRotator MakeRotFromZX(const FVector& Z, const FVector& X);
    UFUNCTION(BlueprintPure)
    static FRotator FindLookAtRotation(const FVector& Start, const FVector& Target);
    UFUNCTION(BlueprintPure)
    static void BreakRotator( FRotator InRot, float& Roll, float& Pitch, float& Yaw);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_RotatorRotator(FRotator A, FRotator B, float ErrorTolerance = 1.e-4f);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_RotatorRotator(FRotator A, FRotator B, float ErrorTolerance = 1.e-4f);
    UFUNCTION(BlueprintPure)
    static FRotator Multiply_RotatorFloat(FRotator A, float B);
    UFUNCTION(BlueprintPure)
    static FRotator ComposeRotators(FRotator A, FRotator B);
    UFUNCTION(BlueprintPure)
    static FRotator NegateRotator(FRotator A);
    UFUNCTION(BlueprintPure)
    static FVector GetForwardVector(FRotator InRot);
    UFUNCTION(BlueprintPure)
    static FVector GetRightVector(FRotator InRot);
    UFUNCTION(BlueprintPure)
    static FVector GetUpVector(FRotator InRot);
    UFUNCTION(BlueprintPure)
    static FVector Conv_RotatorToVector(FRotator InRot);
    UFUNCTION(BlueprintPure)
    static FTransform Conv_RotatorToTransform(const FRotator& InRotator);
    UFUNCTION(BlueprintPure)
    static FRotator RandomRotator(bool bRoll = false);
    UFUNCTION(BlueprintPure)
    static FRotator RLerp(FRotator A, FRotator B, float Alpha, bool bShortestPath);
    UFUNCTION(BlueprintPure)
    static FRotator NormalizedDeltaRotator(FRotator A, FRotator B);
    UFUNCTION(BlueprintPure)
    static FVector4 Matrix_TransformPosition(const FMatrix& M, FVector V);
    UFUNCTION(BlueprintPure)
    static FVector Quat_RotateVector(const FQuat& Q, const FVector& V);
    UFUNCTION(BlueprintPure)
    static FLinearColor LinearColor_Transparent();
    UFUNCTION(BlueprintPure)
    static FLinearColor MakeColor(float R, float G, float B, float A = 1.0f);
    UFUNCTION(BlueprintPure)
    static void BreakColor(FLinearColor InColor, float& R, float& G, float& B, float& A);
    UFUNCTION(BlueprintPure)
    static FLinearColor Conv_DoubleToLinearColor(double InDouble);
    UFUNCTION(BlueprintPure)
    static FLinearColor HSVToRGB(float H, float S, float V, float A = 1.0f);
    UFUNCTION(BlueprintPure)
    static FLinearColor HSVToRGBLinear(FLinearColor HSV);
    UFUNCTION(BlueprintPure)
    static void RGBToHSV(FLinearColor InColor, float& H, float& S, float& V, float& A);
    UFUNCTION(BlueprintPure)
    static FVector Conv_LinearColorToVector(FLinearColor InLinearColor);
    UFUNCTION(BlueprintPure)
    static FColor Conv_LinearColorToColor(FLinearColor InLinearColor, bool InUseSRGB = true);
    UFUNCTION(BlueprintPure)
    static FLinearColor LinearColor_ToNewOpacity(FLinearColor InColor, float InOpacity);
    UFUNCTION(BlueprintPure)
    static FLinearColor LinearColorLerp(FLinearColor A, FLinearColor B, float Alpha);
    UFUNCTION(BlueprintPure)
    static FLinearColor LinearColorLerpUsingHSV(FLinearColor A, FLinearColor B, float Alpha);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_LinearColorLinearColor(FLinearColor A, FLinearColor B);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_LinearColorLinearColor(FLinearColor A, FLinearColor B);
    UFUNCTION(BlueprintPure)
    static FLinearColor Multiply_LinearColorLinearColor(FLinearColor A, FLinearColor B);
    UFUNCTION(BlueprintPure)
    static FPlane MakePlaneFromPointAndNormal(FVector Point, FVector Normal);
    UFUNCTION(BlueprintPure)
    static FDateTime MakeDateTime(int32 Year, int32 Month, int32 Day, int32 Hour = 0, int32 Minute = 0, int32 Second = 0, int32 Millisecond = 0);
    UFUNCTION(BlueprintPure)
    static void BreakDateTime(FDateTime InDateTime, int32& Year, int32& Month, int32& Day, int32& Hour, int32& Minute, int32& Second, int32& Millisecond);
    UFUNCTION(BlueprintPure)
    static FDateTime Add_DateTimeTimespan( FDateTime A, FTimespan B );
    UFUNCTION(BlueprintPure)
    static FDateTime Subtract_DateTimeTimespan(FDateTime A, FTimespan B);
    UFUNCTION(BlueprintPure)
    static FTimespan Subtract_DateTimeDateTime(FDateTime A, FDateTime B);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_DateTimeDateTime( FDateTime A, FDateTime B );
    UFUNCTION(BlueprintPure)
    static bool NotEqual_DateTimeDateTime( FDateTime A, FDateTime B );
    UFUNCTION(BlueprintPure)
    static bool Greater_DateTimeDateTime( FDateTime A, FDateTime B );
    UFUNCTION(BlueprintPure)
    static bool GreaterEqual_DateTimeDateTime( FDateTime A, FDateTime B );
    UFUNCTION(BlueprintPure)
    static bool Less_DateTimeDateTime( FDateTime A, FDateTime B );
    UFUNCTION(BlueprintPure)
    static bool LessEqual_DateTimeDateTime( FDateTime A, FDateTime B );
    UFUNCTION(BlueprintPure)
    static int32 GetMillisecond( FDateTime A );
    UFUNCTION(BlueprintPure)
    static FDateTime DateTimeMaxValue( );
    UFUNCTION(BlueprintPure)
    static FDateTime DateTimeMinValue( );
    UFUNCTION(BlueprintPure)
    static FDateTime Now( );
    UFUNCTION(BlueprintPure)
    static FDateTime UtcNow( );
    UFUNCTION(BlueprintPure)
    static bool DateTimeFromString(FString DateTimeString, FDateTime& Result);
    UFUNCTION(BlueprintPure)
    static double ToUnixTimestampDouble(const FDateTime& Time);
    UFUNCTION(BlueprintPure)
    static FTimespan TimespanZeroValue( );
    UFUNCTION(BlueprintPure)
    static FTimespan MakeTimespan(int32 Days, int32 Hours, int32 Minutes, int32 Seconds, int32 Milliseconds);
    UFUNCTION(BlueprintPure)
    static void BreakTimespan(FTimespan InTimespan, int32& Days, int32& Hours, int32& Minutes, int32& Seconds, int32& Milliseconds);
    UFUNCTION(BlueprintPure)
    static FTimespan Subtract_TimespanTimespan( FTimespan A, FTimespan B );
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_TimespanTimespan( FTimespan A, FTimespan B );
    UFUNCTION(BlueprintPure)
    static bool NotEqual_TimespanTimespan( FTimespan A, FTimespan B );
    UFUNCTION(BlueprintPure)
    static bool Greater_TimespanTimespan( FTimespan A, FTimespan B );
    UFUNCTION(BlueprintPure)
    static bool GreaterEqual_TimespanTimespan( FTimespan A, FTimespan B );
    UFUNCTION(BlueprintPure)
    static bool Less_TimespanTimespan( FTimespan A, FTimespan B );
    UFUNCTION(BlueprintPure)
    static int32 GetDays( FTimespan A );
    UFUNCTION(BlueprintPure)
    static float GetTotalSeconds( FTimespan A );
    UFUNCTION(BlueprintPure)
    static FTimespan FromHours( float Hours );
    UFUNCTION(BlueprintPure)
    static FTimespan FromMilliseconds( float Milliseconds );
    UFUNCTION(BlueprintPure)
    static FTimespan FromSeconds( float Seconds );
    UFUNCTION(BlueprintPure)
    static double Conv_ByteToDouble(uint8 InByte);
    UFUNCTION(BlueprintPure)
    static double Conv_IntToDouble(int32 InInt);
    UFUNCTION(BlueprintPure)
    static int64 Conv_IntToInt64(int32 InInt);
    UFUNCTION(BlueprintPure)
    static uint8 Conv_IntToByte(int32 InInt);
    UFUNCTION(BlueprintPure)
    static int32 Conv_Int64ToInt(int64 InInt);
    UFUNCTION(BlueprintPure)
    static int64 Conv_DoubleToInt64(double InDouble);
    UFUNCTION(BlueprintPure)
    static FVector Conv_IntToVector(int32 InInt);
    UFUNCTION(BlueprintPure)
    static bool Conv_IntToBool(int32 InInt);
    UFUNCTION(BlueprintPure)
    static int32 Conv_BoolToInt(bool InBool);
    UFUNCTION(BlueprintPure)
    static double Conv_BoolToDouble(bool InBool);
    UFUNCTION(BlueprintPure)
    static uint8 Conv_BoolToByte(bool InBool);
    UFUNCTION(BlueprintPure)
    static int32 Conv_ByteToInt(uint8 InByte);
    UFUNCTION(BlueprintPure)
    static int64 Conv_ByteToInt64(uint8 InByte);
    UFUNCTION(BlueprintPure)
    static FLinearColor Conv_ColorToLinearColor(FColor InColor);
    UFUNCTION(BlueprintPure)
    static FVector Conv_DoubleToVector(double InDouble);
    UFUNCTION(BlueprintPure)
    static FVector2D Conv_DoubleToVector2D(double InDouble);
    UFUNCTION(BlueprintPure)
    static FBox MakeBox(FVector Min, FVector Max);
    UFUNCTION(BlueprintPure)
    static void BreakBoxSphereBounds(const FBoxSphereBounds& InBoxSphereBounds, FVector& Origin, FVector& BoxExtent, float& SphereRadius);
    UFUNCTION(BlueprintPure)
    static FRandomStream MakeRandomStream(int32 InitialSeed);
    UFUNCTION(BlueprintPure)
    static FString SelectString(const FString& A, const FString& B, bool bPickA);
    UFUNCTION(BlueprintPure)
    static int32 SelectInt(int32 A, int32 B, bool bPickA);
    UFUNCTION(BlueprintPure)
    static double SelectFloat(double A, double B, bool bPickA);
    UFUNCTION(BlueprintPure)
    static FVector SelectVector(FVector A, FVector B, bool bPickA);
    UFUNCTION(BlueprintPure)
    static FRotator SelectRotator(FRotator A, FRotator B, bool bPickA);
    UFUNCTION(BlueprintPure)
    static FLinearColor SelectColor(FLinearColor A, FLinearColor B, bool bPickA);
    UFUNCTION(BlueprintPure)
    static FTransform SelectTransform(const FTransform& A, const FTransform& B, bool bPickA);
    UFUNCTION(BlueprintPure)
    static UObject* SelectObject(UObject* A, UObject* B, bool bSelectA);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_ObjectObject(class UObject* A, class UObject* B);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_ObjectObject(class UObject* A, class UObject* B);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_ClassClass(class UClass* A, class UClass* B);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_ClassClass(class UClass* A, class UClass* B);
    UFUNCTION(BlueprintPure)
    static bool ClassIsChildOf(TSubclassOf<class UObject> TestClass, TSubclassOf<class UObject> ParentClass);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_NameName(FName A, FName B);
    UFUNCTION(BlueprintPure)
    static bool NotEqual_NameName(FName A, FName B);
    UFUNCTION(BlueprintPure)
    static FTransform MakeTransform(FVector Location, FRotator Rotation, FVector Scale = FVector(1,1,1));
    UFUNCTION(BlueprintPure)
    static void BreakTransform(const FTransform& InTransform, FVector& Location, FRotator& Rotation, FVector& Scale);
    UFUNCTION(BlueprintPure)
    static bool EqualEqual_TransformTransform(const FTransform& A, const FTransform& B);
    UFUNCTION(BlueprintPure)
    static FTransform ComposeTransforms(const FTransform& A, const FTransform& B);
    UFUNCTION(BlueprintPure)
    static FVector TransformLocation(const FTransform& T, FVector Location);
    UFUNCTION(BlueprintPure)
    static FVector TransformDirection(const FTransform& T, FVector Direction);
    UFUNCTION(BlueprintPure)
    static FVector InverseTransformLocation(const FTransform& T, FVector Location);
    UFUNCTION(BlueprintPure)
    static FVector InverseTransformDirection(const FTransform& T, FVector Direction);
    UFUNCTION(BlueprintPure)
    static FRotator InverseTransformRotation(const FTransform& T, FRotator Rotation);
    UFUNCTION(BlueprintPure)
    static FTransform MakeRelativeTransform(const FTransform& A, const FTransform& RelativeTo);
    UFUNCTION(BlueprintPure)
    static FTransform InvertTransform(const FTransform& T);
    UFUNCTION(BlueprintPure)
    static FTransform TLerp(const FTransform& A, const FTransform& B, float Alpha, TEnumAsByte<ELerpInterpolationMode::Type> InterpMode = ELerpInterpolationMode::QuatInterp);
    UFUNCTION(BlueprintPure)
    static double FInterpTo(double Current, double Target, double DeltaTime, double InterpSpeed);
    UFUNCTION(BlueprintPure)
    static double FInterpTo_Constant(double Current, double Target, double DeltaTime, double InterpSpeed);
    UFUNCTION(BlueprintPure)
    static FRotator RInterpTo(FRotator Current, FRotator Target, float DeltaTime, float InterpSpeed);
    UFUNCTION(BlueprintPure)
    static FRotator RInterpTo_Constant(FRotator Current, FRotator Target, float DeltaTime, float InterpSpeed);
    UFUNCTION(BlueprintPure)
    static int32 RandomIntegerFromStream(const FRandomStream& Stream, int32 Max);
    UFUNCTION(BlueprintPure)
    static int32 RandomIntegerInRangeFromStream(const FRandomStream& Stream, int32 Min, int32 Max);
    UFUNCTION(BlueprintPure)
    static bool RandomBoolFromStream(const FRandomStream& Stream);
    UFUNCTION(BlueprintPure)
    static float RandomFloatFromStream(const FRandomStream& Stream);
    UFUNCTION(BlueprintPure)
    static float RandomFloatInRangeFromStream(const FRandomStream& Stream, float Min, float Max);
    UFUNCTION(BlueprintPure)
    static FVector RandomUnitVectorFromStream(const FRandomStream& Stream);
    UFUNCTION(BlueprintCallable)
    static void SetRandomStreamSeed(UPARAM(ref) FRandomStream& Stream, int32 NewSeed);
    UFUNCTION(BlueprintPure)
    static bool IsPointInBox(FVector Point, FVector BoxOrigin, FVector BoxExtent);
    UFUNCTION(BlueprintPure)
    static bool IsPointInBoxWithTransform(FVector Point, const FTransform& BoxWorldTransform, FVector BoxExtent);
    UFUNCTION(BlueprintPure)
    static bool LinePlaneIntersection(const FVector& LineStart, const FVector& LineEnd, const FPlane& APlane, float& T, FVector& Intersection);
    UFUNCTION(BlueprintPure)
    static bool LinePlaneIntersection_OriginNormal(const FVector& LineStart, const FVector& LineEnd, FVector PlaneOrigin, FVector PlaneNormal, float& T, FVector& Intersection);

};

}
