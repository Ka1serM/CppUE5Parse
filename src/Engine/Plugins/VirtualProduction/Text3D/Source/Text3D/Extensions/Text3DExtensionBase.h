#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "Text3DExtensionBase.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UText3DExtensionBase : public UObject {
    GENERATED_BODY()
public:
    explicit UText3DExtensionBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
