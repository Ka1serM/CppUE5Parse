#include "ue5parse/runtime/Engine/Kismet/KismetMaterialLibrary.h"

#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/Materials/MaterialInstanceDynamic.h"

// UE's Kismet/KismetMaterialLibrary.cpp (UE 5.3).

namespace ue5parse::runtime {

UMaterialInstanceDynamic* UKismetMaterialLibrary::CreateDynamicMaterialInstance(UObject* WorldContextObject, UMaterialInterface* Parent,
                                                                                const FName OptionalName, const EMIDCreationFlags CreationFlags) {
    if (Parent == nullptr)
        return nullptr;
    UObject* Outer = CreationFlags == EMIDCreationFlags::Transient || WorldContextObject == nullptr ? GetTransientPackage() : WorldContextObject;
    return UMaterialInstanceDynamic::Create(Parent, Outer, OptionalName);
}

}
