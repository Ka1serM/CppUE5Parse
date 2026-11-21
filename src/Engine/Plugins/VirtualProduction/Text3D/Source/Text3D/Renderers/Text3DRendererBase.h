#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "Text3DRendererBase.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UText3DRendererBase : public UObject {
    GENERATED_BODY()
public:
    explicit UText3DRendererBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
