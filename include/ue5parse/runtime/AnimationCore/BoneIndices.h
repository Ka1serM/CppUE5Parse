#pragma once

#include "ue5parse/runtime/Core/Templates/TypeHash.h"

// The bone index types (UE's AnimationCore BoneIndices.h): an index into a compact pose, a mesh pose or a skeleton
// pose, kept apart by type.

namespace ue5parse::runtime {

using FBoneIndexType = uint16;

struct FBoneIndexBase {
    FBoneIndexBase() = default;
    int32 GetInt() const { return BoneIndex; }
    bool IsRootBone() const { return BoneIndex == 0; }
    bool IsValid() const { return BoneIndex != INDEX_NONE; }
    explicit operator int32() const { return BoneIndex; }
    explicit operator bool() const { return IsValid(); }
    friend uint32 GetTypeHash(const FBoneIndexBase& Index) { return static_cast<uint32>(Index.BoneIndex); }

protected:
    int32 BoneIndex = INDEX_NONE;
};

template <class RealBoneIndexType>
struct FBoneIndexWithOperators : public FBoneIndexBase {
    friend bool operator==(const RealBoneIndexType& Lhs, const RealBoneIndexType& Rhs) { return Lhs.GetInt() == Rhs.GetInt(); }
    friend bool operator<(const RealBoneIndexType& Lhs, const RealBoneIndexType& Rhs) { return Lhs.GetInt() < Rhs.GetInt(); }
    friend bool operator<=(const RealBoneIndexType& Lhs, const RealBoneIndexType& Rhs) { return Lhs.GetInt() <= Rhs.GetInt(); }
    friend bool operator>(const RealBoneIndexType& Lhs, const RealBoneIndexType& Rhs) { return Lhs.GetInt() > Rhs.GetInt(); }
    friend bool operator>=(const RealBoneIndexType& Lhs, const RealBoneIndexType& Rhs) { return Lhs.GetInt() >= Rhs.GetInt(); }
    friend bool operator==(const RealBoneIndexType& Lhs, int32 Rhs) { return Lhs.GetInt() == Rhs; }
    friend bool operator<(const RealBoneIndexType& Lhs, int32 Rhs) { return Lhs.GetInt() < Rhs; }
    friend bool operator<=(const RealBoneIndexType& Lhs, int32 Rhs) { return Lhs.GetInt() <= Rhs; }
    friend bool operator>(const RealBoneIndexType& Lhs, int32 Rhs) { return Lhs.GetInt() > Rhs; }
    friend bool operator>=(const RealBoneIndexType& Lhs, int32 Rhs) { return Lhs.GetInt() >= Rhs; }
    RealBoneIndexType& operator++() {
        ++BoneIndex;
        return *static_cast<RealBoneIndexType*>(this);
    }
    RealBoneIndexType& operator--() {
        --BoneIndex;
        return *static_cast<RealBoneIndexType*>(this);
    }
};

struct FCompactPoseBoneIndex : public FBoneIndexWithOperators<FCompactPoseBoneIndex> {
    explicit FCompactPoseBoneIndex(int32 InBoneIndex) { BoneIndex = InBoneIndex; }
};

struct FMeshPoseBoneIndex : public FBoneIndexWithOperators<FMeshPoseBoneIndex> {
    explicit FMeshPoseBoneIndex(int32 InBoneIndex) { BoneIndex = InBoneIndex; }
};

struct FSkeletonPoseBoneIndex : public FBoneIndexWithOperators<FSkeletonPoseBoneIndex> {
    explicit FSkeletonPoseBoneIndex(int32 InBoneIndex) { BoneIndex = InBoneIndex; }
};

}
