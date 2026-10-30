#pragma once

#include "ue5parse/runtime/Engine/Curves/RichCurve.h"

#include <algorithm>

#include "AnimCurveTypes.generated.h"

namespace ue5parse::runtime {

// FAnimCurveBase: a named animation curve (UE's Animation/AnimCurveTypes.h).
USTRUCT()
struct FAnimCurveBase {
    GENERATED_BODY()
    UPROPERTY()
    FName CurveName;
    UPROPERTY()
    int32 CurveTypeFlags = 0;

    FName GetName() const { return CurveName; }
};

USTRUCT()
struct FFloatCurve : public FAnimCurveBase {
    GENERATED_BODY()
    UPROPERTY()
    FRichCurve FloatCurve;

    float Evaluate(float CurrentTime) const { return FloatCurve.Eval(CurrentTime); }
};

// FRawCurveTracks: the float curves of an animation as authored.
USTRUCT()
struct FRawCurveTracks {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FFloatCurve> FloatCurves;
};


namespace UE::Anim {

enum class ECurveElementFlags : uint8 { None = 0, Material = 1 << 0, MorphTarget = 1 << 1 };
inline ECurveElementFlags operator|(ECurveElementFlags A, ECurveElementFlags B) { return ECurveElementFlags(uint8(A) | uint8(B)); }
inline ECurveElementFlags& operator|=(ECurveElementFlags& A, ECurveElementFlags B) { return A = A | B; }

// FCurveElement: one named value of a blended curve.
struct FCurveElement {
    FName Name;
    float Value = 0;
    ECurveElementFlags Flags = ECurveElementFlags::None;
};

}

// FBlendedCurve: the named curve values of a pose (UE 5.3's TBaseBlendedCurve over a named value array). The elements
// are kept sorted by name, so the blends are merges (UE's FNamedValueArrayUtils::Union); a missing element reads as 0.
struct FBlendedCurve {
    using ElementType = UE::Anim::FCurveElement;

    int32 Num() const { return Elements.Num(); }
    bool IsEmpty() const { return Elements.Num() == 0; }
    void Empty() { Elements.Empty(); }
    const TArray<ElementType>& GetElements() const { return Elements; }

    void Set(FName InName, float InValue);
    void SetFlags(FName InName, UE::Anim::ECurveElementFlags InFlags);
    float Get(FName InName) const {
        const ElementType* Element = Find(InName);
        return Element ? Element->Value : 0.0f;
    }
    float Get(FName InName, bool& OutHasElement, float InDefaultValue = 0.0f) const {
        const ElementType* Element = Find(InName);
        OutHasElement = Element != nullptr;
        return Element ? Element->Value : InDefaultValue;
    }
    void InvalidateCurveWeight(FName InName);
    template <class Visit>
    void ForEachElement(Visit&& InVisit) const {
        for (const ElementType& Element : Elements) InVisit(Element);
    }

    void CopyFrom(const FBlendedCurve& Other) { Elements = Other.Elements; }
    void Override(const FBlendedCurve& CurveToOverrideFrom, float Weight = 1.0f);
    // Lerp(A, B, Alpha) into this curve.
    void Lerp(const FBlendedCurve& A, const FBlendedCurve& B, float Alpha);
    void LerpTo(const FBlendedCurve& Other, float Alpha);
    void ConvertToAdditive(const FBlendedCurve& BaseCurve);
    void Accumulate(const FBlendedCurve& AdditiveCurve, float Weight);
    // The values of CurveToCombine replace this curve's (UE's Combine); CombinePreserved only adds the missing ones.
    void Combine(const FBlendedCurve& CurveToCombine);
    void CombinePreserved(const FBlendedCurve& CurveToCombine);
    // The larger (smaller) value of the two curves for each name.
    void UseMaxValue(const FBlendedCurve& CurveToCombine);
    void UseMinValue(const FBlendedCurve& CurveToCombine);
    // Multiplies each value by the scale.
    void Mult(float Scale) {
        for (ElementType& Element : Elements) Element.Value *= Scale;
    }
    // Keeps only the names for which the predicate returns false.
    template <class Predicate>
    void RemoveByPredicate(Predicate&& InPredicate) {
        Elements.RemoveAll([&InPredicate](const ElementType& Element) { return InPredicate(Element); });
    }

private:
    static bool NameLess(const FName& A, const FName& B) {
        return A.getHash() != B.getHash() ? A.getHash() < B.getHash() : A.getNumber() < B.getNumber();
    }
    const ElementType* Find(FName InName) const;
    ElementType* Find(FName InName) { return const_cast<ElementType*>(static_cast<const FBlendedCurve*>(this)->Find(InName)); }
    // Calls Combine(Out, This, Other, bHasThis, bHasOther) for every name of this curve or Other, in name order.
    template <class Combiner>
    void Union(const FBlendedCurve& Other, Combiner&& Combine);

    TArray<ElementType> Elements;
};

template <class Combiner>
void FBlendedCurve::Union(const FBlendedCurve& Other, Combiner&& Combine) {
    TArray<ElementType> Result;
    Result.Reserve(FMath::Max(Elements.Num(), Other.Elements.Num()));
    int32 A = 0, B = 0;
    const ElementType Missing;
    while (A < Elements.Num() || B < Other.Elements.Num()) {
        const bool bTakeA = A < Elements.Num() && (B >= Other.Elements.Num() || !NameLess(Other.Elements[B].Name, Elements[A].Name));
        const bool bTakeB = B < Other.Elements.Num() && (A >= Elements.Num() || !NameLess(Elements[A].Name, Other.Elements[B].Name));
        ElementType Out = bTakeA ? Elements[A] : Other.Elements[B];
        if (!bTakeA) Out.Value = 0, Out.Flags = UE::Anim::ECurveElementFlags::None;
        Combine(Out, bTakeA ? Elements[A] : Missing, bTakeB ? Other.Elements[B] : Missing, bTakeA, bTakeB);
        Result.Add(Out);
        A += bTakeA;
        B += bTakeB;
    }
    Elements = std::move(Result);
}

}
