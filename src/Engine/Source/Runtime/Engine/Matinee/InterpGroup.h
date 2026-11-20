#pragma once

#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/Engine/Matinee/InterpTrack.h"

#include "InterpGroup.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UInterpGroup : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UInterpTrack>> InterpTracks;
    UPROPERTY()
    FName GroupName = FName("UInterpGroup");
    UPROPERTY()
    FColor GroupColor = FColor(100, 80, 200, 255);
    UPROPERTY()
    bool bCollapsed = false;
    UPROPERTY()
    bool bIsFolder = false;
    UPROPERTY()
    bool bIsParented = false;
};

}
