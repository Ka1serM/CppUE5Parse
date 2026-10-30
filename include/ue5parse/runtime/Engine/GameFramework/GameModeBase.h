#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Info.h"

#include "GameModeBase.generated.h"

namespace ue5parse::runtime {

// AGameModeBase: the rules of the game; it exists on the server only (UE's GameFramework/GameModeBase.h).
UCLASS()
class AGameModeBase : public AInfo {
    GENERATED_BODY()
};

}
