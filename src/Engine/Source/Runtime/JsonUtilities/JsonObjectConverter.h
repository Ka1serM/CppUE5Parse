#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnknownProperties.h"
#include "Engine/Source/Runtime/Json/Serialization/JsonWriter.h"

namespace ue5parse::runtime {

class FProperty;

class FJsonObjectConverter {
public:
    static void UObjectToJsonObject(const UObject* Object, FJsonWriter& Writer);
    static void UStructToJsonObject(const UStruct* Struct, const void* Data, FJsonWriter& Writer);

private:
    struct FContext {
        FJsonWriter& Writer;
        const FUnknownProperties* UnknownProperties;
    };

    static void WriteStruct(const FContext& Context, const UStruct* Struct, const void* Data, const UStruct* DefaultsStruct, const void* Defaults);
    static void WriteValue(const FContext& Context, const FProperty* Property, const void* Value);
    static void WriteObjectReference(FJsonWriter& Writer, const UObject* Object);
};

}
