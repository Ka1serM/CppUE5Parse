#pragma once

#include "CppUE5Parse/Core/FGuid.h"
#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class FArchive;
class FProperty;

enum class EPropertyTagSerializeType : uint8 {
    Unknown,
    Skipped,
    Property,
    BinaryOrNative,
};

struct FPropertyTypeNameNode {
    FName Name;
    int32 InnerCount = 0;
};

class FPropertyTypeName {
public:
    FName GetName() const { return Nodes.Num() > 0 ? Nodes[0].Name : FName(); }
    int32 GetParameterCount() const { return Nodes.Num() > 0 ? Nodes[0].InnerCount : 0; }
    FName GetParameterName(int32 ParameterIndex) const;
    FPropertyTypeName GetParameter(int32 ParameterIndex) const;

    void Reset() { Nodes.Reset(); }
    void AddNode(FName Name, int32 InnerCount) { Nodes.Add(FPropertyTypeNameNode{Name, InnerCount}); }
    void SetParameterCount(int32 Count) { Nodes[0].InnerCount = Count; }
    void Serialize(FArchive& Ar);

private:
    int32 FindParameterNode(int32 ParameterIndex) const;

    TArray<FPropertyTypeNameNode> Nodes;
};

struct FPropertyTag {
    FName Name;
    FName Type;
    FPropertyTypeName TypeName;
    int32 Size = 0;
    int32 ArrayIndex = 0;
    uint8 BoolVal = 0;
    bool HasPropertyGuid = false;
    FGuid PropertyGuid;
    EPropertyTagSerializeType SerializeType = EPropertyTagSerializeType::Unknown;

    void SerializeTaggedProperty(FArchive& Ar, const FProperty* Property, uint8* Value) const;
};

FArchive& operator<<(FArchive& Ar, FPropertyTag& Tag);

}
