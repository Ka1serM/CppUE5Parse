#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

#include <string>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

enum EPropertyTagFlags : uint8 {
    HasArrayIndex = 0x01,
    HasPropertyGuid = 0x02,
    HasPropertyExtensions = 0x04,
    HasBinaryOrNativeSerialize = 0x08,
    BoolTrue = 0x10,
    SkippedSerialize = 0x20,
};

enum EPropertyTagExtension : uint8 {
    OverridableInformation = 0x02,
    HasExternalsObjects = 0x04,
};

constexpr FName NAME_StructProperty("StructProperty");
constexpr FName NAME_BoolProperty("BoolProperty");
constexpr FName NAME_ByteProperty("ByteProperty");
constexpr FName NAME_EnumProperty("EnumProperty");
constexpr FName NAME_ArrayProperty("ArrayProperty");
constexpr FName NAME_OptionalProperty("OptionalProperty");
constexpr FName NAME_SetProperty("SetProperty");
constexpr FName NAME_MapProperty("MapProperty");

//! Older tags store an enum as a path name; the full type name keeps only the object name.
FName EnumObjectName(FName EnumName) {
    std::string_view Text = EnumName.getText();
    std::size_t Separator = Text.find_last_of(".:");
    if (Separator == std::string_view::npos)
        return EnumName;
    return FName(Text.substr(Separator + 1));
}

void SerializePropertyExtensions(FArchive& Ar) {
    uint8 Extensions = 0;
    Ar << Extensions;
    if (Extensions & OverridableInformation) {
        uint8 OverrideOperation = 0;
        bool bExperimentalOverridableLogic = false;
        Ar << OverrideOperation << bExperimentalOverridableLogic;
    }
    if (Extensions & HasExternalsObjects) {
        bool bExperimentalExternalObjects = false;
        Ar << bExperimentalExternalObjects;
    }
}

void LoadPropertyTagNoFullType(FArchive& Ar, FPropertyTag& Tag) {
    const FPackageFileVersion Version = Ar.UEVer();
    Ar << Tag.Type << Tag.Size << Tag.ArrayIndex;
    Tag.TypeName.Reset();
    Tag.TypeName.AddNode(Tag.Type, 0);
    const auto AddParameters = [&Tag](std::initializer_list<FName> Names) {
        Tag.TypeName.SetParameterCount(static_cast<int32>(Names.size()));
        for (FName Name : Names)
            Tag.TypeName.AddNode(Name, 0);
    };
    if (Tag.Type == NAME_StructProperty) {
        FName StructName;
        Ar << StructName;
        if (Version >= EUnrealEngineObjectUE4Version::STRUCT_GUID_IN_PROPERTY_TAG) {
            FGuid StructGuid;
            Ar << StructGuid;
        }
        AddParameters({StructName});
    } else if (Tag.Type == NAME_BoolProperty) {
        Ar << Tag.BoolVal;
    } else if (Tag.Type == NAME_ByteProperty) {
        FName EnumName;
        Ar << EnumName;
        if (!EnumName.isNone())
            AddParameters({EnumObjectName(EnumName)});
    } else if (Tag.Type == NAME_EnumProperty) {
        FName EnumName;
        Ar << EnumName;
        AddParameters({EnumObjectName(EnumName), NAME_ByteProperty});
    } else if (Tag.Type == NAME_ArrayProperty) {
        FName InnerType;
        if (Version >= EUnrealEngineObjectUE4Version::ARRAY_PROPERTY_INNER_TAGS)
            Ar << InnerType;
        AddParameters({InnerType});
    } else if (Tag.Type == NAME_OptionalProperty) {
        FName InnerType;
        Ar << InnerType;
        AddParameters({InnerType});
    } else if (Version >= EUnrealEngineObjectUE4Version::PROPERTY_TAG_SET_MAP_SUPPORT) {
        if (Tag.Type == NAME_SetProperty) {
            FName InnerType;
            Ar << InnerType;
            AddParameters({InnerType});
        } else if (Tag.Type == NAME_MapProperty) {
            FName InnerType;
            FName ValueType;
            Ar << InnerType << ValueType;
            AddParameters({InnerType, ValueType});
        }
    }
    if (Version >= EUnrealEngineObjectUE4Version::PROPERTY_GUID_IN_PROPERTY_TAG) {
        uint8 HasPropertyGuidByte = 0;
        Ar << HasPropertyGuidByte;
        Tag.HasPropertyGuid = HasPropertyGuidByte != 0;
        if (Tag.HasPropertyGuid)
            Ar << Tag.PropertyGuid;
    }
    if (Version >= EUnrealEngineObjectUE5Version::PROPERTY_TAG_EXTENSION_AND_OVERRIDABLE_SERIALIZATION)
        SerializePropertyExtensions(Ar);
    Tag.SerializeType = EPropertyTagSerializeType::Unknown;
}

}

int32 FPropertyTypeName::FindParameterNode(int32 ParameterIndex) const {
    if (ParameterIndex < 0 || ParameterIndex >= GetParameterCount())
        return INDEX_NONE;
    int32 Node = 1;
    for (int32 Skip = ParameterIndex; Skip > 0; Skip--, Node++)
        Skip += Nodes[Node].InnerCount;
    return Node;
}

FName FPropertyTypeName::GetParameterName(int32 ParameterIndex) const {
    const int32 Node = FindParameterNode(ParameterIndex);
    return Node == INDEX_NONE ? FName() : Nodes[Node].Name;
}

FPropertyTypeName FPropertyTypeName::GetParameter(int32 ParameterIndex) const {
    FPropertyTypeName Parameter;
    const int32 First = FindParameterNode(ParameterIndex);
    if (First == INDEX_NONE)
        return Parameter;
    for (int32 Node = First, Remaining = 1; Remaining > 0; Node++, Remaining--) {
        Parameter.Nodes.Add(Nodes[Node]);
        Remaining += Nodes[Node].InnerCount;
    }
    return Parameter;
}

void FPropertyTypeName::Serialize(FArchive& Ar) {
    Nodes.Reset();
    for (int32 Remaining = 1; Remaining > 0 && !Ar.IsError(); Remaining--) {
        FPropertyTypeNameNode Node;
        Ar << Node.Name << Node.InnerCount;
        if (Node.InnerCount < 0) {
            Ar.SetError(ErrorCode::InvalidData, "FPropertyTypeName.InnerCount");
            return;
        }
        Remaining += Node.InnerCount;
        Nodes.Add(Node);
    }
}

FArchive& operator<<(FArchive& Ar, FPropertyTag& Tag) {
    Ar << Tag.Name;
    if (Tag.Name.isNone() || Ar.IsError())
        return Ar;
    Tag.BoolVal = 0;
    Tag.HasPropertyGuid = false;
    if (Ar.UEVer() < EUnrealEngineObjectUE5Version::PROPERTY_TAG_COMPLETE_TYPE_NAME) {
        LoadPropertyTagNoFullType(Ar, Tag);
        return Ar;
    }
    Tag.TypeName.Serialize(Ar);
    Tag.Type = Tag.TypeName.GetName();
    uint8 Flags = 0;
    Ar << Tag.Size << Flags;
    Tag.SerializeType = (Flags & SkippedSerialize)               ? EPropertyTagSerializeType::Skipped
                        : (Flags & HasBinaryOrNativeSerialize) ? EPropertyTagSerializeType::BinaryOrNative
                                                               : EPropertyTagSerializeType::Property;
    Tag.BoolVal = (Flags & BoolTrue) ? 1 : 0;
    Tag.HasPropertyGuid = (Flags & HasPropertyGuid) != 0;
    Tag.ArrayIndex = 0;
    if (Flags & HasArrayIndex)
        Ar << Tag.ArrayIndex;
    if (Tag.HasPropertyGuid)
        Ar << Tag.PropertyGuid;
    if (Flags & HasPropertyExtensions)
        SerializePropertyExtensions(Ar);
    return Ar;
}

void FPropertyTag::SerializeTaggedProperty(FArchive& Ar, const FProperty* Property, uint8* Value) const {
    const int64 StartOfProperty = Ar.Tell();
    if (const FBoolProperty* Bool = CastField<FBoolProperty>(Property))
        Bool->SetPropertyValue(Value, BoolVal != 0);
    else
        Property->SerializeItem(Ar, Value);
    if (Ar.IsError())
        return;
    const int64 Read = Ar.Tell() - StartOfProperty;
    if (Read == Size)
        return;
    ReportLoadProblem("load.tag_size", FString(Name.toString()) + FString(" of type ") + FString(Type.toString()) + FString(" read ")
                                           + FString(std::to_string(Read)) + FString(" of ") + FString(std::to_string(Size)) + FString(" bytes"));
    Ar.SetError(ErrorCode::InvalidData, "FPropertyTag.Size");
}

}
