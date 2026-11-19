#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "InputCoreTypes.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FKey {
    GENERATED_BODY()
    UPROPERTY()
    FName KeyName;
};

namespace ETouchIndex {

UENUM()
enum Type : int32 { Touch1, Touch2, Touch3, Touch4, Touch5, Touch6, Touch7, Touch8, Touch9, Touch10, CursorPointerIndex, MAX_TOUCHES };

}

USTRUCT()
struct FTouchId {
    GENERATED_BODY()
    UPROPERTY()
    FInputDeviceId DeviceId;
    UPROPERTY()
    TEnumAsByte<ETouchIndex::Type> TouchIndex = ETouchIndex::Touch1;
};

}
