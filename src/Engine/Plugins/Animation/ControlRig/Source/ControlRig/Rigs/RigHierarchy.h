#pragma once

#include <memory>

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyElements.h"
#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyMetadata.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "RigHierarchy.generated.h"

namespace ue5parse::runtime {

UCLASS()
class URigHierarchy : public UObject {
    GENERATED_BODY()
public:
    ~URigHierarchy() override;

    void Serialize(FArchive& Ar) override;
    void Load(FArchive& Ar);

    int32 Num() const { return Elements.Num(); }
    FRigBaseElement* Get(int32 InIndex) const { return Elements.IsValidIndex(InIndex) ? Elements[InIndex] : nullptr; }
    FRigBaseElement* Find(const FRigElementKey& InKey) const;
    template <class T>
    T* Find(const FRigElementKey& InKey) const {
        return Cast<T>(Find(InKey));
    }

    FRigBaseMetadata* GetMetadataForElement(FRigBaseElement* InElement, const FName& InName, ERigMetadataType InType);

    struct FMetadataStorage {
        //! UE owns raw metadata pointers; the port shares them so that the storage can be copied.
        TMap<FName, std::shared_ptr<FRigBaseMetadata>> MetadataMap;

        void Reset();
        void Serialize(FArchive& Ar);

        friend FArchive& operator<<(FArchive& Ar, FMetadataStorage& Storage) {
            Storage.Serialize(Ar);
            return Ar;
        }
    };

    TArray<FRigBaseElement*> Elements;
    TMap<FRigElementKey, int32> ElementIndexLookup;
    TArray<FMetadataStorage> ElementMetadata;
    //! UE resolves these script structs and loads each component; the port keeps the names and skips the components, as UE does for a struct it cannot find.
    TArray<FString> ComponentScriptStructNames;
    TMap<FRigHierarchyKey, FRigHierarchyKey> PreviousHierarchyNameMap;
    TMap<FRigHierarchyKey, FRigHierarchyKey> PreviousHierarchyParentMap;

private:
    FRigBaseElement* MakeElement(FArchive& Ar, ERigElementType InElementType);
    void Reset();
};

}
