#pragma once

#include "ue5parse/runtime/Core/Math/Transform.h"

namespace ue5parse::runtime {

// TBox: an axis aligned box (UE's Box.h). IsValid is a byte in memory and in cooked data.
template <class T>
struct TBox {
    TVector<T> Min;
    TVector<T> Max;
    uint8 IsValid = 0;

    constexpr TBox() = default;
    constexpr TBox(const TVector<T>& InMin, const TVector<T>& InMax) : Min(InMin), Max(InMax), IsValid(1) {}

    bool operator==(const TBox& Other) const { return Min == Other.Min && Max == Other.Max; }
    TBox& operator+=(const TVector<T>& Other) {
        if (IsValid) {
            Min = Min.ComponentMin(Other);
            Max = Max.ComponentMax(Other);
        } else {
            Min = Max = Other;
            IsValid = 1;
        }
        return *this;
    }
    TBox operator+(const TVector<T>& Other) const { return TBox(*this) += Other; }
    TBox& operator+=(const TBox& Other) {
        if (IsValid && Other.IsValid) {
            Min = Min.ComponentMin(Other.Min);
            Max = Max.ComponentMax(Other.Max);
        } else if (Other.IsValid) {
            *this = Other;
        }
        return *this;
    }
    TBox operator+(const TBox& Other) const { return TBox(*this) += Other; }
    TVector<T> GetCenter() const { return (Min + Max) * T(0.5); }
    TVector<T> GetExtent() const { return (Max - Min) * T(0.5); }
    TVector<T> GetSize() const { return Max - Min; }
    void GetCenterAndExtents(TVector<T>& Center, TVector<T>& Extents) const {
        Extents = GetExtent();
        Center = Min + Extents;
    }
    T GetVolume() const { return (Max.X - Min.X) * (Max.Y - Min.Y) * (Max.Z - Min.Z); }
    bool IsInside(const TVector<T>& In) const {
        return In.X > Min.X && In.X < Max.X && In.Y > Min.Y && In.Y < Max.Y && In.Z > Min.Z && In.Z < Max.Z;
    }
    bool IsInsideOrOn(const TVector<T>& In) const {
        return In.X >= Min.X && In.X <= Max.X && In.Y >= Min.Y && In.Y <= Max.Y && In.Z >= Min.Z && In.Z <= Max.Z;
    }
    bool Intersect(const TBox& Other) const {
        if (Min.X > Other.Max.X || Other.Min.X > Max.X) return false;
        if (Min.Y > Other.Max.Y || Other.Min.Y > Max.Y) return false;
        if (Min.Z > Other.Max.Z || Other.Min.Z > Max.Z) return false;
        return true;
    }
    TBox ExpandBy(T W) const { return TBox(Min - TVector<T>(W), Max + TVector<T>(W)); }
    TBox ShiftBy(const TVector<T>& Offset) const { return TBox(Min + Offset, Max + Offset); }
    TBox TransformBy(const TTransform<T>& M) const;
    TBox TransformBy(const TMatrix<T>& M) const;
    static TBox BuildAABB(const TVector<T>& Origin, const TVector<T>& Extent) { return TBox(Origin - Extent, Origin + Extent); }
};

using FBox = TBox<double>;
using FBox3d = TBox<double>;
using FBox3f = TBox<float>;

template <class T>
struct TBox2 {
    TVector2<T> Min;
    TVector2<T> Max;
    bool bIsValid = false;

    constexpr TBox2() = default;
    constexpr TBox2(const TVector2<T>& InMin, const TVector2<T>& InMax) : Min(InMin), Max(InMax), bIsValid(true) {}
    TVector2<T> GetCenter() const { return (Min + Max) * T(0.5); }
    TVector2<T> GetExtent() const { return (Max - Min) * T(0.5); }
    TVector2<T> GetSize() const { return Max - Min; }
    bool IsInside(const TVector2<T>& P) const { return P.X > Min.X && P.X < Max.X && P.Y > Min.Y && P.Y < Max.Y; }
    bool operator==(const TBox2& Other) const { return Min == Other.Min && Max == Other.Max && bIsValid == Other.bIsValid; }
};

using FBox2D = TBox2<double>;
using FBox2f = TBox2<float>;

template <class T>
struct TSphere {
    TVector<T> Center;
    T W = 0;
    constexpr TSphere() = default;
    constexpr TSphere(const TVector<T>& InV, T InW) : Center(InV), W(InW) {}
    bool IsInside(const TVector<T>& In, T Tolerance = T(UE_KINDA_SMALL_NUMBER)) const {
        return (Center - In).SizeSquared() <= FMath::Square(W + Tolerance);
    }
    bool operator==(const TSphere& Other) const { return Center == Other.Center && W == Other.W; }
};

using FSphere = TSphere<double>;
using FSphere3f = TSphere<float>;

// FBoxSphereBounds: a box and a sphere around the same origin (UE's BoxSphereBounds.h).
template <class T>
struct TBoxSphereBounds {
    TVector<T> Origin;
    TVector<T> BoxExtent;
    T SphereRadius = 0;

    constexpr TBoxSphereBounds() = default;
    TBoxSphereBounds(const TVector<T>& InOrigin, const TVector<T>& InBoxExtent, T InSphereRadius)
        : Origin(InOrigin), BoxExtent(InBoxExtent), SphereRadius(InSphereRadius) {}
    explicit TBoxSphereBounds(const TBox<T>& Box) {
        Box.GetCenterAndExtents(Origin, BoxExtent);
        SphereRadius = BoxExtent.Size();
    }
    TBox<T> GetBox() const { return TBox<T>(Origin - BoxExtent, Origin + BoxExtent); }
    TSphere<T> GetSphere() const { return TSphere<T>(Origin, SphereRadius); }
    TBoxSphereBounds ExpandBy(T ExpandAmount) const {
        return TBoxSphereBounds(Origin, BoxExtent + TVector<T>(ExpandAmount), SphereRadius + ExpandAmount);
    }
    TBoxSphereBounds TransformBy(const TTransform<T>& M) const;
    TBoxSphereBounds TransformBy(const TMatrix<T>& M) const;
    bool operator==(const TBoxSphereBounds& Other) const {
        return Origin == Other.Origin && BoxExtent == Other.BoxExtent && SphereRadius == Other.SphereRadius;
    }
    TBoxSphereBounds operator+(const TBoxSphereBounds& Other) const {
        TBox<T> BoundingBox = GetBox() + Other.GetBox();
        TBoxSphereBounds Result(BoundingBox);
        Result.SphereRadius = FMath::Min(Result.SphereRadius,
                                         FMath::Max((Origin - Result.Origin).Size() + SphereRadius,
                                                    (Other.Origin - Result.Origin).Size() + Other.SphereRadius));
        return Result;
    }
};

using FBoxSphereBounds = TBoxSphereBounds<double>;
using FBoxSphereBounds3f = TBoxSphereBounds<float>;

template <class T>
TBox<T> TBox<T>::TransformBy(const TMatrix<T>& M) const {
    if (!IsValid) return TBox();
    const TVector<T> Origin = GetCenter();
    const TVector<T> Extent = GetExtent();
    TVector<T> NewOrigin = M.TransformPosition(Origin);
    TVector<T> NewExtent = M.TransformVector(TVector<T>(Extent.X, 0, 0)).GetAbs() + M.TransformVector(TVector<T>(0, Extent.Y, 0)).GetAbs() +
                           M.TransformVector(TVector<T>(0, 0, Extent.Z)).GetAbs();
    return TBox(NewOrigin - NewExtent, NewOrigin + NewExtent);
}

template <class T>
TBox<T> TBox<T>::TransformBy(const TTransform<T>& M) const {
    return TransformBy(M.ToMatrixWithScale());
}

template <class T>
TBoxSphereBounds<T> TBoxSphereBounds<T>::TransformBy(const TMatrix<T>& M) const {
    TBoxSphereBounds Result;
    Result.Origin = M.TransformPosition(Origin);
    Result.BoxExtent = M.TransformVector(TVector<T>(BoxExtent.X, 0, 0)).GetAbs() + M.TransformVector(TVector<T>(0, BoxExtent.Y, 0)).GetAbs() +
                       M.TransformVector(TVector<T>(0, 0, BoxExtent.Z)).GetAbs();
    const T MaxRadius = std::sqrt(FMath::Max3(M.GetScaledAxis(0).SizeSquared(), M.GetScaledAxis(1).SizeSquared(), M.GetScaledAxis(2).SizeSquared()));
    Result.SphereRadius = FMath::Min(Result.BoxExtent.Size(), SphereRadius * MaxRadius);
    return Result;
}

template <class T>
TBoxSphereBounds<T> TBoxSphereBounds<T>::TransformBy(const TTransform<T>& M) const {
    return TransformBy(M.ToMatrixWithScale());
}

}
