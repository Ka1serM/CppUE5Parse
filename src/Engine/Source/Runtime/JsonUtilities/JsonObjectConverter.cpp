#include "Engine/Source/Runtime/JsonUtilities/JsonObjectConverter.h"

#include <string>

#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

bool IsIdenticalToDefault(const FProperty* Property, const void* Data, const UStruct* DefaultsStruct, const void* Defaults) {
    if (Defaults == nullptr || !Property->IsInContainer(DefaultsStruct->GetPropertiesSize()))
        return false;
    for (int32 Index = 0; Index < Property->ArrayDim; Index++)
        if (!Property->Identical(Property->ContainerPtrToValuePtr<void>(Data, Index), Property->ContainerPtrToValuePtr<void>(Defaults, Index)))
            return false;
    return true;
}

std::string GetUnknownPropertyName(const FUnknownProperty& Unknown) {
    std::string Name = Unknown.GetProperty()->GetName().ToUtf8();
    if (Unknown.GetArrayIndex() == 0)
        return Name;
    return Name + "[" + std::to_string(Unknown.GetArrayIndex()) + "]";
}

void WriteEnumValue(FJsonWriter& Writer, const UEnum* Enum, int64 Value) {
    const FName Name = Enum != nullptr ? Enum->GetNameByValue(Value) : FName();
    if (Name.isNone())
        Writer.WriteValue(Value);
    else
        Writer.WriteValue(Name.toString());
}

void WriteNumericValue(FJsonWriter& Writer, const FNumericProperty* Numeric, const void* Value) {
    if (Numeric->IsA<FFloatProperty>())
        Writer.WriteValue(*static_cast<const float*>(Value));
    else if (Numeric->IsFloatingPoint())
        Writer.WriteValue(Numeric->GetFloatingPointPropertyValue(Value));
    else if (const UEnum* Enum = Numeric->GetIntPropertyEnum())
        WriteEnumValue(Writer, Enum, Numeric->GetSignedIntPropertyValue(Value));
    else if (Numeric->IsA<FUInt64Property>())
        Writer.WriteValue(Numeric->GetUnsignedIntPropertyValue(Value));
    else
        Writer.WriteValue(Numeric->GetSignedIntPropertyValue(Value));
}

void WriteDelegate(FJsonWriter& Writer, const FScriptDelegate& Delegate) {
    Writer.WriteObjectStart();
    const UObject* Object = Delegate.GetUObjectEvenIfUnreachable();
    Writer.WriteValue("Object", Object != nullptr ? Object->GetPathName().ToUtf8() : std::string("None"));
    Writer.WriteValue("FunctionName", Delegate.GetFunctionName().toString());
    Writer.WriteObjectEnd();
}

}

void FJsonObjectConverter::UObjectToJsonObject(const UObject* Object, FJsonWriter& Writer) {
    const UClass* Class = Object->GetClass();
    Writer.WriteObjectStart();
    Writer.WriteValue("Type", Class->GetName().ToUtf8());
    Writer.WriteValue("Name", Object->GetName().ToUtf8());
    if (const UObject* Outer = Object->GetOuter(); Outer != nullptr && !Outer->IsA<UPackage>())
        Writer.WriteValue("Outer", Outer->GetName().ToUtf8());
    Writer.WriteValue("Class", Class->GetPathName().ToUtf8());
    const FLinkerLoad* Linker = Object->GetLinker();
    const FContext Context{Writer, Linker != nullptr ? Linker->GetUnknownProperties(Object->GetLinkerIndex()) : nullptr};
    const UObject* Archetype = Object->GetArchetype();
    Writer.WriteIdentifierPrefix("Properties");
    WriteStruct(Context, Class, Object, Archetype != nullptr ? Archetype->GetClass() : nullptr, Archetype);
    Writer.WriteObjectEnd();
}

void FJsonObjectConverter::UStructToJsonObject(const UStruct* Struct, const void* Data, FJsonWriter& Writer) {
    WriteStruct(FContext{Writer, nullptr}, Struct, Data, nullptr, nullptr);
}

void FJsonObjectConverter::WriteStruct(const FContext& Context, const UStruct* Struct, const void* Data, const UStruct* DefaultsStruct,
                                       const void* Defaults) {
    FJsonWriter& Writer = Context.Writer;
    Writer.WriteObjectStart();
    for (const FProperty* Property = Struct->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext) {
        if (!Property->ShouldSerializeValue() || IsIdenticalToDefault(Property, Data, DefaultsStruct, Defaults))
            continue;
        Writer.WriteIdentifierPrefix(Property->GetName().ToUtf8());
        if (Property->ArrayDim == 1) {
            WriteValue(Context, Property, Property->ContainerPtrToValuePtr<void>(Data));
            continue;
        }
        Writer.WriteArrayStart();
        for (int32 Index = 0; Index < Property->ArrayDim; Index++)
            WriteValue(Context, Property, Property->ContainerPtrToValuePtr<void>(Data, Index));
        Writer.WriteArrayEnd();
    }
    if (Context.UnknownProperties != nullptr) {
        for (const FUnknownProperty& Unknown : Context.UnknownProperties->FindByContainer(Data)) {
            Writer.WriteIdentifierPrefix(GetUnknownPropertyName(Unknown));
            WriteValue(Context, Unknown.GetProperty(), Unknown.GetValue());
        }
    }
    Writer.WriteObjectEnd();
}

void FJsonObjectConverter::WriteObjectReference(FJsonWriter& Writer, const UObject* Object) {
    if (Object == nullptr) {
        Writer.WriteNull();
        return;
    }
    Writer.WriteObjectStart();
    Writer.WriteValue("ObjectName", (Object->GetClass()->GetName() + FString("'") + Object->GetName() + FString("'")).ToUtf8());
    Writer.WriteValue("ObjectPath", Object->GetPathName().ToUtf8());
    Writer.WriteObjectEnd();
}

void FJsonObjectConverter::WriteValue(const FContext& Context, const FProperty* Property, const void* Value) {
    FJsonWriter& Writer = Context.Writer;
    if (const FBoolProperty* Bool = CastField<FBoolProperty>(Property)) {
        Writer.WriteValue(Bool->GetPropertyValue(Value));
    } else if (const FNumericProperty* Numeric = CastField<FNumericProperty>(Property)) {
        WriteNumericValue(Writer, Numeric, Value);
    } else if (const FEnumProperty* Enum = CastField<FEnumProperty>(Property)) {
        WriteEnumValue(Writer, Enum->Enum, Enum->UnderlyingProp->GetSignedIntPropertyValue(Value));
    } else if (CastField<FNameProperty>(Property) != nullptr) {
        Writer.WriteValue(static_cast<const FName*>(Value)->toString());
    } else if (CastField<FStrProperty>(Property) != nullptr) {
        Writer.WriteValue(static_cast<const FString*>(Value)->ToUtf8());
    } else if (CastField<FTextProperty>(Property) != nullptr) {
        Writer.WriteValue(static_cast<const FText*>(Value)->ToString().ToUtf8());
    } else if (CastField<FSoftObjectProperty>(Property) != nullptr) {
        Writer.WriteValue(static_cast<const FSoftObjectPtr*>(Value)->GetUniqueID().ToString().ToUtf8());
    } else if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property)) {
        WriteObjectReference(Writer, Object->GetObjectPropertyValue(Value));
    } else if (CastField<FInterfaceProperty>(Property) != nullptr) {
        WriteObjectReference(Writer, static_cast<const FScriptInterface*>(Value)->GetObject());
    } else if (const FStructProperty* Struct = CastField<FStructProperty>(Property)) {
        WriteStruct(Context, Struct->Struct, Value, nullptr, nullptr);
    } else if (const FArrayProperty* Array = CastField<FArrayProperty>(Property)) {
        FScriptArrayHelper Helper(Array, Value);
        Writer.WriteArrayStart();
        for (int32 Index = 0; Index < Helper.Num(); Index++)
            WriteValue(Context, Array->Inner, Helper.GetRawPtr(Index));
        Writer.WriteArrayEnd();
    } else if (const FSetProperty* Set = CastField<FSetProperty>(Property)) {
        FScriptSetHelper Helper(Set, Value);
        Writer.WriteArrayStart();
        for (int32 Index = 0; Index < Helper.GetMaxIndex(); Index++)
            if (Helper.IsValidIndex(Index))
                WriteValue(Context, Set->ElementProp, Helper.GetElementPtr(Index));
        Writer.WriteArrayEnd();
    } else if (const FMapProperty* Map = CastField<FMapProperty>(Property)) {
        FScriptMapHelper Helper(Map, Value);
        Writer.WriteArrayStart();
        for (int32 Index = 0; Index < Helper.GetMaxIndex(); Index++) {
            if (!Helper.IsValidIndex(Index))
                continue;
            Writer.WriteObjectStart();
            Writer.WriteIdentifierPrefix("Key");
            WriteValue(Context, Map->KeyProp, Helper.GetKeyPtr(Index));
            Writer.WriteIdentifierPrefix("Value");
            WriteValue(Context, Map->ValueProp, Helper.GetValuePtr(Index));
            Writer.WriteObjectEnd();
        }
        Writer.WriteArrayEnd();
    } else if (const FOptionalProperty* Optional = CastField<FOptionalProperty>(Property)) {
        if (const void* Set = Optional->GetValuePointerForReadIfSet(Value))
            WriteValue(Context, Optional->ValueProperty, Set);
        else
            Writer.WriteNull();
    } else if (CastField<FDelegateProperty>(Property) != nullptr) {
        WriteDelegate(Writer, *static_cast<const FScriptDelegate*>(Value));
    } else if (const FMulticastDelegateProperty* Multicast = CastField<FMulticastDelegateProperty>(Property)) {
        Writer.WriteArrayStart();
        if (const FMulticastScriptDelegate* Delegates = Multicast->GetMulticastDelegate(Value))
            for (const FScriptDelegate& Delegate : Delegates->InvocationList)
                WriteDelegate(Writer, Delegate);
        Writer.WriteArrayEnd();
    } else if (CastField<FFieldPathProperty>(Property) != nullptr) {
        Writer.WriteValue(static_cast<const FFieldPath*>(Value)->ToString().ToUtf8());
    } else {
        Writer.WriteNull();
    }
}

}
