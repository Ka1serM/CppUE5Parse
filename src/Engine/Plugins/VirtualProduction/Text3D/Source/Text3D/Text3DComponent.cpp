#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Text3DComponent.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DDefaultCharacterExtension.h"
#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DDefaultGeometryExtension.h"
#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DDefaultLayoutExtension.h"
#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DDefaultMaterialExtension.h"
#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DDefaultRenderingExtension.h"
#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DDefaultStyleExtension.h"
#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DDefaultTokenExtension.h"

namespace ue5parse::runtime {

UText3DComponent::UText3DComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    const EGame Game = GetRuntimeProvider().getGame();
    if (Game >= EGame::GAME_UE4_25 && Game < EGame::GAME_UE5_6) {
        TextRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("TextRoot"));
        if (Game >= EGame::GAME_UE5_3)
            TextRoot->SetupAttachment(this);
        return;
    }
    if (Game < EGame::GAME_UE5_6)
        return;
    if (Game >= EGame::GAME_UE5_7) {
        TokenExtension = ObjectInitializer.CreateDefaultSubobject<UText3DDefaultTokenExtension>(FName("TemplateExtension"));
        StyleExtension = ObjectInitializer.CreateDefaultSubobject<UText3DDefaultStyleExtension>(FName("StyleExtension"));
    }
    MaterialExtension = ObjectInitializer.CreateDefaultSubobject<UText3DDefaultMaterialExtension>(FName("MaterialExtension"));
    GeometryExtension = ObjectInitializer.CreateDefaultSubobject<UText3DDefaultGeometryExtension>(FName("GeometryExtension"));
    LayoutExtension = ObjectInitializer.CreateDefaultSubobject<UText3DDefaultLayoutExtension>(FName("LayoutExtension"));
    RenderingExtension = ObjectInitializer.CreateDefaultSubobject<UText3DDefaultRenderingExtension>(FName("RenderingExtension"));
    CharacterExtension = ObjectInitializer.CreateDefaultSubobject<UText3DDefaultCharacterExtension>(FName("CharacterExtension"));
}

}
