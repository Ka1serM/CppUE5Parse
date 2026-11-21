#pragma once

#include <optional>

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

namespace ue5parse::runtime {

struct FControlRigOverridePropertyInfo {
    UObject* Owner = nullptr;
    TArray<FName> FieldPath;
    UObject* FieldOwner = nullptr;
    int32 ArrayIndex = -1;

    friend FArchive& operator<<(FArchive& Ar, FControlRigOverridePropertyInfo& Info) {
        return Ar << Info.Owner << Info.FieldPath << Info.FieldOwner << Info.ArrayIndex;
    }
};

struct FControlRigOverrideValue {
    std::optional<FName> SubjectKey;
    TArray<FControlRigOverridePropertyInfo> Properties;
    FSoftObjectPath OwnerStructPath;
    std::optional<FString> Path;
    int32 PropertySize = 0;
    uint32 PropertyHash = 0;
    TArray<uint8> Data;

    friend FArchive& operator<<(FArchive& Ar, FControlRigOverrideValue& Value);
};

struct FControlRigOverrideContainer {
    bool bUsesKeyForSubject = false;
    TArray<FControlRigOverrideValue> Values;

    bool Serialize(FArchive& Ar) {
        Ar << bUsesKeyForSubject << Values;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FControlRigOverrideContainer> : TStructOpsTypeTraitsBase2<FControlRigOverrideContainer> {
    enum { WithSerializer = true };
};

}
