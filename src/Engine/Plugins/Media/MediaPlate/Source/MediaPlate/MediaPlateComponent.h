#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "MediaPlateComponent.generated.h"

namespace ue5parse::runtime {

class UMediaPlayer;
class UMediaPlaylist;
class UMediaTexture;
class UStaticMeshComponent;

UCLASS()
class UMediaPlateComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UMediaPlateComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UMediaPlaylist> MediaPlaylist = nullptr;
    UPROPERTY()
    TObjectPtr<UMediaPlaylist> ActivePlaylist = nullptr;
    UPROPERTY()
    TObjectPtr<UMediaTexture> MediaTexture = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMediaTexture>> MediaTextures;
    UPROPERTY()
    TObjectPtr<UMediaPlayer> MediaPlayer = nullptr;
};

}
