#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

namespace ue5parse::runtime {

struct FInstancedStructCustomVersion {
    static constexpr FGuid GUID{0xE21E1CAA, 0xAF47425E, 0x89BF6AD4, 0x4C44A8BB};
    enum Type : int32 {
        CustomVersionAdded = 0,
    };
};

void SerializeStructInstance(FArchive& Ar, const UScriptStruct* Struct, void* Memory, int32 SerialSize);

class FInstancedStruct {
public:
    FInstancedStruct() = default;
    FInstancedStruct(const FInstancedStruct& Other) { *this = Other; }
    FInstancedStruct& operator=(const FInstancedStruct& Other);
    ~FInstancedStruct() { Reset(); }

    void InitializeAs(const UScriptStruct* InScriptStruct);
    void Reset();
    uint8* GetMutableMemory() { return StructMemory; }
    const uint8* GetMemory() const { return StructMemory; }
    const UScriptStruct* GetScriptStruct() const { return ScriptStruct; }
    bool IsValid() const { return ScriptStruct != nullptr && StructMemory != nullptr; }
    bool IsChildOf(const UScriptStruct* Base) const;
    bool operator==(const FInstancedStruct& Other) const;
    bool Serialize(FArchive& Ar);

protected:
    const UScriptStruct* ScriptStruct = nullptr;
    uint8* StructMemory = nullptr;
};

template <>
struct TStructOpsTypeTraits<FInstancedStruct> : TStructOpsTypeTraitsBase2<FInstancedStruct> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

//! The layout is FInstancedStruct's, so a TInstancedStruct UPROPERTY is an FInstancedStruct property.
template <class BaseStructType>
class TInstancedStruct : public FInstancedStruct {
public:
    BaseStructType* GetMutablePtr() { return IsChildOf(BaseStructType::StaticStruct()) ? reinterpret_cast<BaseStructType*>(StructMemory) : nullptr; }
    const BaseStructType* GetPtr() const { return IsChildOf(BaseStructType::StaticStruct()) ? reinterpret_cast<const BaseStructType*>(StructMemory) : nullptr; }
};

}
