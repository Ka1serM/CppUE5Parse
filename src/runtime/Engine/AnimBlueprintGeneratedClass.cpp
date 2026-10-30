#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"

#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

const UAnimBlueprintGeneratedClass* UAnimBlueprintGeneratedClass::GetRootAnimClass() const {
    const UAnimBlueprintGeneratedClass* Root = this;
    while (const auto* Parent = Cast<UAnimBlueprintGeneratedClass>(Root->GetSuperClass())) Root = Parent;
    return Root;
}

TArray<FStructProperty*> UAnimBlueprintGeneratedClass::GetAnimNodeProperties() const {
    TArray<FStructProperty*> Nodes;
    const UAnimBlueprintGeneratedClass* Root = GetRootAnimClass();
    for (FField* Field = Root->ChildProperties; Field != nullptr; Field = Field->Next) {
        auto* Property = CastField<FStructProperty>(Field);
        if (Property == nullptr || Property->Struct == nullptr) continue;
        for (const UStruct* Type = Property->Struct; Type != nullptr; Type = Type->GetSuperStruct()) {
            if (Type->GetFName() != FName("AnimNode_Base")) continue;
            Nodes.Add(Property);
            break;
        }
    }
    return Nodes;
}

namespace {

const void* FindNodeTypeData(const UAnimBlueprintGeneratedClass* Class, const UScriptStruct* NodeType) {
    const auto* MapProperty = CastField<FMapProperty>(Class->GetClass()->FindPropertyByName(FName("NodeTypeMap")));
    if (!MapProperty || !CastField<FObjectPropertyBase>(MapProperty->KeyProp) ||
        !CastField<FStructProperty>(MapProperty->ValueProp)) return nullptr;
    FScriptMapHelper Map(MapProperty, MapProperty->ContainerPtrToValuePtr<void>(Class));
    for (int32 Index = 0; Index < Map.GetMaxIndex(); ++Index) {
        if (!Map.IsValidIndex(Index)) continue;
        const auto* Key = CastField<FObjectPropertyBase>(MapProperty->KeyProp);
        if (Key->GetObjectPropertyValue(Map.GetKeyPtr(Index)) == NodeType) return Map.GetValuePtr(Index);
    }
    return nullptr;
}

}

int32 UAnimBlueprintGeneratedClass::GetAnimNodePropertyIndex(const UScriptStruct* NodeType, FName PropertyName) const {
    const auto* MapProperty = CastField<FMapProperty>(GetClass()->FindPropertyByName(FName("NodeTypeMap")));
    const void* Data = FindNodeTypeData(this, NodeType);
    if (!Data || !MapProperty) return INDEX_NONE;
    const auto* StructProperty = CastField<FStructProperty>(MapProperty->ValueProp);
    const auto* NameMapProperty = CastField<FMapProperty>(StructProperty->Struct->FindPropertyByName(FName("NameToIndexMap")));
    if (!NameMapProperty || !CastField<FNameProperty>(NameMapProperty->KeyProp) ||
        !CastField<FNumericProperty>(NameMapProperty->ValueProp)) return INDEX_NONE;
    FScriptMapHelper Names(NameMapProperty, NameMapProperty->ContainerPtrToValuePtr<void>(Data));
    if (uint8* Value = Names.FindValueFromHash(&PropertyName))
        return static_cast<int32>(CastField<FNumericProperty>(NameMapProperty->ValueProp)->GetSignedIntPropertyValue(Value));
    return INDEX_NONE;
}

int32 UAnimBlueprintGeneratedClass::GetAnimNodePropertyCount(const UScriptStruct* NodeType) const {
    const auto* MapProperty = CastField<FMapProperty>(GetClass()->FindPropertyByName(FName("NodeTypeMap")));
    const void* Data = FindNodeTypeData(this, NodeType);
    if (!Data || !MapProperty) return 0;
    const auto* StructProperty = CastField<FStructProperty>(MapProperty->ValueProp);
    const auto* CountProperty = CastField<FNumericProperty>(StructProperty->Struct->FindPropertyByName(FName("NumProperties")));
    return CountProperty ? static_cast<int32>(CountProperty->GetSignedIntPropertyValue(
        CountProperty->ContainerPtrToValuePtr<void>(Data))) : 0;
}

const void* UAnimBlueprintGeneratedClass::GetAnimNodeValue(const FAnimNodeData& NodeData,
        const UScriptStruct* NodeType, FName PropertyName, const UObject* Instance,
        const FProperty*& ValueProperty) const {
    ValueProperty = nullptr;
    const int32 FieldIndex = GetAnimNodePropertyIndex(NodeType, PropertyName);
    const int32 ValueIndex = NodeData.GetDataIndex(FieldIndex);
    if (ValueIndex == INDEX_NONE) return nullptr;

    const_cast<UAnimBlueprintGeneratedClass*>(this)->GetDefaultObject();
    const UStruct* DataStruct = SparseClassDataStruct;
    const void* Data = GetSparseClassData();
    if (NodeData.IsInstanceData(FieldIndex)) {
        if (!Instance) return nullptr;
        const FStructProperty* MutableData = nullptr;
        for (const UStruct* Type = Instance->GetClass(); Type && !MutableData; Type = Type->GetSuperStruct()) {
            for (FField* Field = Type->ChildProperties; Field; Field = Field->Next) {
                auto* StructProperty = CastField<FStructProperty>(Field);
                if (!StructProperty) continue;
                for (const UStruct* StructType = StructProperty->Struct; StructType; StructType = StructType->GetSuperStruct()) {
                    if (StructType->GetFName() != FName("AnimBlueprintMutableData")) continue;
                    MutableData = StructProperty;
                    break;
                }
                if (MutableData) break;
            }
        }
        if (!MutableData) return nullptr;
        DataStruct = MutableData->Struct;
        Data = MutableData->ContainerPtrToValuePtr<void>(Instance);
    }
    if (!DataStruct || !Data) return nullptr;
    int32 Index = 0;
    for (const UStruct* Type = DataStruct; Type; Type = Type->GetSuperStruct()) {
        for (FField* Field = Type->ChildProperties; Field; Field = Field->Next) {
            auto* Property = CastField<FProperty>(Field);
            if (!Property) continue;
            if (Index++ != ValueIndex) continue;
            ValueProperty = Property;
            return Property->ContainerPtrToValuePtr<void>(Data);
        }
    }
    return nullptr;
}

const void* UAnimBlueprintGeneratedClass::GetAnimNodeFieldValue(int32 NodeIndex, FName PropertyName,
        const UObject* Instance, const FProperty*& ValueProperty) const {
    ValueProperty = nullptr;
    const TArray<FStructProperty*> Nodes = GetAnimNodeProperties();
    if (!Nodes.IsValidIndex(NodeIndex)) return nullptr;
    const FStructProperty* Node = Nodes[NodeIndex];
    if (!Instance) Instance = const_cast<UAnimBlueprintGeneratedClass*>(this)->GetDefaultObject();
    if (!Instance || !Instance->GetClass()->IsChildOf(GetRootAnimClass())) return nullptr;
    ValueProperty = Node->Struct->FindPropertyByName(PropertyName);
    if (!ValueProperty) return nullptr;
    return ValueProperty->ContainerPtrToValuePtr<void>(Node->ContainerPtrToValuePtr<void>(Instance));
}

int32 UAnimBlueprintGeneratedClass::GetAnimNodeLinkID(int32 NodeIndex, FName PropertyName, const UObject* Instance) const {
    const FProperty* LinkProperty = nullptr;
    const void* Link = GetAnimNodeFieldValue(NodeIndex, PropertyName, Instance, LinkProperty);
    const auto* StructProperty = CastField<FStructProperty>(LinkProperty);
    if (!Link || !StructProperty || !StructProperty->Struct) return INDEX_NONE;
    const auto* IdProperty = CastField<FNumericProperty>(StructProperty->Struct->FindPropertyByName(FName("LinkID")));
    if (!IdProperty) return INDEX_NONE;
    return static_cast<int32>(IdProperty->GetSignedIntPropertyValue(IdProperty->ContainerPtrToValuePtr<void>(Link)));
}

TArray<int32> UAnimBlueprintGeneratedClass::GetAnimNodeLinkIDs(int32 NodeIndex, FName PropertyName,
        const UObject* Instance) const {
    TArray<int32> Links;
    const FProperty* ArrayField = nullptr;
    const void* Array = GetAnimNodeFieldValue(NodeIndex, PropertyName, Instance, ArrayField);
    const auto* ArrayProperty = CastField<FArrayProperty>(ArrayField);
    if (!Array || !ArrayProperty) return Links;
    const auto* LinkProperty = CastField<FStructProperty>(ArrayProperty->Inner);
    if (!LinkProperty || !LinkProperty->Struct) return Links;
    const auto* IdProperty = CastField<FNumericProperty>(LinkProperty->Struct->FindPropertyByName(FName("LinkID")));
    if (!IdProperty) return Links;
    FScriptArrayHelper Elements(ArrayProperty, Array);
    for (int32 Index = 0; Index < Elements.Num(); ++Index)
        Links.Add(static_cast<int32>(IdProperty->GetSignedIntPropertyValue(
            IdProperty->ContainerPtrToValuePtr<void>(Elements.GetRawPtr(Index)))));
    return Links;
}

const FExposedValueHandler* UAnimBlueprintGeneratedClass::GetExposedValueHandler(int32 NodeIndex) const {
    const_cast<UAnimBlueprintGeneratedClass*>(this)->GetDefaultObject();
    if (!SparseClassDataStruct || !GetSparseClassData()) return nullptr;
    for (const UStruct* Type = SparseClassDataStruct; Type; Type = Type->GetSuperStruct()) {
        for (FField* Field = Type->ChildProperties; Field; Field = Field->Next) {
            const auto* Subsystem = CastField<FStructProperty>(Field);
            if (!Subsystem || !Subsystem->Struct) continue;
            const auto* Handlers = CastField<FArrayProperty>(Subsystem->Struct->FindPropertyByName(FName("ExposedValueHandlers")));
            if (!Handlers) continue;
            const auto* Handler = CastField<FStructProperty>(Handlers->Inner);
            if (!Handler || Handler->Struct != FExposedValueHandler::StaticStruct()) continue;
            const void* SubsystemData = Subsystem->ContainerPtrToValuePtr<void>(GetSparseClassData());
            FScriptArrayHelper Elements(Handlers, Handlers->ContainerPtrToValuePtr<void>(SubsystemData));
            if (!Elements.IsValidIndex(NodeIndex)) return nullptr;
            return reinterpret_cast<const FExposedValueHandler*>(Elements.GetRawPtr(NodeIndex));
        }
    }
    return nullptr;
}

const FPropertyAccessLibrary* UAnimBlueprintGeneratedClass::GetPropertyAccessLibrary() const {
    const_cast<UAnimBlueprintGeneratedClass*>(this)->GetDefaultObject();
    if (!SparseClassDataStruct || !GetSparseClassData()) return nullptr;
    for (const UStruct* Type = SparseClassDataStruct; Type; Type = Type->GetSuperStruct()) {
        for (FField* Field = Type->ChildProperties; Field; Field = Field->Next) {
            const auto* Subsystem = CastField<FStructProperty>(Field);
            if (!Subsystem || !Subsystem->Struct) continue;
            const auto* Library = CastField<FStructProperty>(Subsystem->Struct->FindPropertyByName(FName("Library")));
            if (!Library || Library->Struct != FPropertyAccessLibrary::StaticStruct()) continue;
            const void* SubsystemData = Subsystem->ContainerPtrToValuePtr<void>(GetSparseClassData());
            return Library->ContainerPtrToValuePtr<FPropertyAccessLibrary>(SubsystemData);
        }
    }
    return nullptr;
}

}
