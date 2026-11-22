#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "PropertyDataWithObjects.generated.h"

namespace ue5parse::runtime {

class FLinkerLoad;

struct FPropertyDataObjectContainer {
    TArray<UObject*> ObjectStore;
    TArray<FSoftObjectPath> SoftObjectPathStore;

    friend FArchive& operator<<(FArchive& Ar, FPropertyDataObjectContainer& Container);
};

//! The Outer Worlds 2 saves names as strings and objects as indices into the stores of the data.
class FOW2ObjectsArchive : public FMemoryReaderView {
public:
    FOW2ObjectsArchive(std::span<const std::byte> InBytes, FArchive& Outer, const FPropertyDataObjectContainer& InObjects, bool bInHasVersion);
    ~FOW2ObjectsArchive() override;
    FOW2ObjectsArchive(const FOW2ObjectsArchive&) = delete;
    FOW2ObjectsArchive& operator=(const FOW2ObjectsArchive&) = delete;

    static const FOW2ObjectsArchive* Find(const FArchive& Ar);
    bool HasVersion() const { return bHasVersion; }

    using FArchive::operator<<;
    FArchive& operator<<(FName& Value) override;
    FArchive& operator<<(UObject*& Value) override;
    FArchive& operator<<(FSoftObjectPath& Value) override;
    FArchive& operator<<(FSoftObjectPtr& Value) override;
    FArchive& operator<<(FWeakObjectPtr& Value) override;
    FLinkerLoad* GetLinker() override { return Linker; }

private:
    FLinkerLoad* Linker;
    const FPropertyDataObjectContainer& Objects;
    bool bHasVersion;
    const FOW2ObjectsArchive* Previous;
};

//! Reads a tagged property list that has no struct into a transient struct with one property for each tag.
void SerializeTaggedPropertiesWithoutStruct(FArchive& Ar, FInstancedStruct& Value);
//! Reads a struct by its name, or as a tagged property list when the struct is not known.
void SerializeStructFallback(FArchive& Ar, FName StructName, FInstancedStruct& Value);
//! Reads the property list of The Outer Worlds 2 where each property keeps its own data and object stores.
void SerializeOW2PropertyTags(FArchive& Ar, FInstancedStruct& Value);

USTRUCT()
struct FPropertyDataWithObjects {
    GENERATED_BODY()

    UPROPERTY()
    FInstancedStruct Properties;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FPropertyDataWithObjects> : TStructOpsTypeTraitsBase2<FPropertyDataWithObjects> {
    enum { WithSerializer = true };
};

}
