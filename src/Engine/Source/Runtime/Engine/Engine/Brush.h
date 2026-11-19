#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "Brush.generated.h"

namespace ue5parse::runtime {

class UModel;
class UBrushComponent;

UENUM()
enum ECsgOper : int32 { CSG_Active, CSG_Add, CSG_Subtract, CSG_Intersect, CSG_Deintersect, CSG_None, CSG_MAX };

UENUM()
enum EBrushType : int32 { Brush_Default, Brush_Add, Brush_Subtract, Brush_MAX };

USTRUCT()
struct FGeomSelection {
    GENERATED_BODY()
    UPROPERTY()
    int32 Type = 0;
    UPROPERTY()
    int32 Index = 0;
    UPROPERTY()
    int32 SelectionIndex = 0;
};

UCLASS()
class ABrush : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<ECsgOper> CsgOper_DEPRECATED = CSG_Active;
    UPROPERTY()
    TEnumAsByte<EBrushType> BrushType = Brush_Default;
    UPROPERTY()
    FColor BrushColor;
    UPROPERTY()
    int32 PolyFlags = 0;
    UPROPERTY()
    uint8 bColored : 1 = false;
    UPROPERTY()
    uint8 bSolidWhenSelected : 1 = false;
    UPROPERTY()
    uint8 bPlaceableFromClassBrowser : 1 = false;
    UPROPERTY()
    uint8 bNotForClientOrServer : 1 = false;
    UPROPERTY()
    TObjectPtr<UModel> Brush = nullptr;
    UPROPERTY()
    TObjectPtr<UBrushComponent> BrushComponent = nullptr;
    UPROPERTY()
    uint8 bInManipulation : 1 = false;
    UPROPERTY()
    TArray<FGeomSelection> SavedSelections;

    explicit ABrush(const FObjectInitializer& ObjectInitializer);

    UBrushComponent* GetBrushComponent() const { return BrushComponent; }
};

}
