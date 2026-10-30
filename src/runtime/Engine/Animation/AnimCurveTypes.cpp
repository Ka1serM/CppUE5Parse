#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"

// UE's FBlendedCurve operations (AnimCurveTypes.h, UE 5.3).

namespace ue5parse::runtime {

namespace {
// UE's FAnimWeight::IsRelevant.
bool IsRelevant(float Weight) { return Weight > ZERO_ANIMWEIGHT_THRESH; }
}

const FBlendedCurve::ElementType* FBlendedCurve::Find(FName InName) const {
    const ElementType* Begin = Elements.GetData();
    const ElementType* End = Begin + Elements.Num();
    const ElementType* Found = std::lower_bound(Begin, End, InName, [](const ElementType& E, const FName& N) { return NameLess(E.Name, N); });
    return Found != End && Found->Name == InName ? Found : nullptr;
}

void FBlendedCurve::Set(FName InName, float InValue) {
    if (ElementType* Element = Find(InName)) {
        Element->Value = InValue;
        return;
    }
    int32 Index = 0;
    while (Index < Elements.Num() && NameLess(Elements[Index].Name, InName)) ++Index;
    Elements.Insert({InName, InValue, UE::Anim::ECurveElementFlags::None}, Index);
}

void FBlendedCurve::SetFlags(FName InName, UE::Anim::ECurveElementFlags InFlags) {
    if (Find(InName) == nullptr) Set(InName, 0.0f);
    Find(InName)->Flags = InFlags;
}

void FBlendedCurve::InvalidateCurveWeight(FName InName) {
    if (const ElementType* Element = Find(InName)) Elements.RemoveAt(static_cast<int32>(Element - Elements.GetData()));
}

void FBlendedCurve::Override(const FBlendedCurve& CurveToOverrideFrom, float Weight) {
    CopyFrom(CurveToOverrideFrom);
    if (!FMath::IsNearlyEqual(Weight, 1.0f))
        for (ElementType& Element : Elements) Element.Value *= Weight;
}

void FBlendedCurve::Lerp(const FBlendedCurve& A, const FBlendedCurve& B, float Alpha) {
    if (!IsRelevant(FMath::Abs(Alpha))) {
        Override(A);
    } else if (!IsRelevant(FMath::Abs(Alpha - 1.0f))) {
        Override(B);
    } else {
        CopyFrom(A);
        Union(B, [Alpha](ElementType& Out, const ElementType& InA, const ElementType& InB, bool, bool) {
            Out.Value = FMath::Lerp(InA.Value, InB.Value, Alpha);
            Out.Flags = InA.Flags | InB.Flags;
        });
    }
}

void FBlendedCurve::LerpTo(const FBlendedCurve& Other, float Alpha) {
    if (!IsRelevant(FMath::Abs(Alpha))) return;
    if (!IsRelevant(FMath::Abs(Alpha - 1.0f))) {
        Override(Other);
        return;
    }
    Union(Other, [Alpha](ElementType& Out, const ElementType& InThis, const ElementType& InOther, bool, bool) {
        Out.Value = FMath::Lerp(InThis.Value, InOther.Value, Alpha);
        Out.Flags = InThis.Flags | InOther.Flags;
    });
}

void FBlendedCurve::ConvertToAdditive(const FBlendedCurve& BaseCurve) {
    Union(BaseCurve, [](ElementType& Out, const ElementType& InThis, const ElementType& InBase, bool, bool) {
        Out.Value = InThis.Value - InBase.Value;
        Out.Flags = InThis.Flags | InBase.Flags;
    });
}

void FBlendedCurve::Accumulate(const FBlendedCurve& AdditiveCurve, float Weight) {
    if (!IsRelevant(Weight)) return;
    Union(AdditiveCurve, [Weight](ElementType& Out, const ElementType& InThis, const ElementType& InAdditive, bool, bool) {
        Out.Value = InThis.Value + InAdditive.Value * Weight;
        Out.Flags = InThis.Flags | InAdditive.Flags;
    });
}

void FBlendedCurve::Combine(const FBlendedCurve& CurveToCombine) {
    Union(CurveToCombine, [](ElementType& Out, const ElementType&, const ElementType& InOther, bool, bool bHasOther) {
        if (bHasOther) Out.Value = InOther.Value, Out.Flags = InOther.Flags;
    });
}

void FBlendedCurve::CombinePreserved(const FBlendedCurve& CurveToCombine) {
    Union(CurveToCombine, [](ElementType& Out, const ElementType&, const ElementType& InOther, bool bHasThis, bool) {
        if (!bHasThis) Out.Value = InOther.Value, Out.Flags = InOther.Flags;
    });
}

}
