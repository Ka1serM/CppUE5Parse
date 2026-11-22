#pragma once

#include "Engine/Source/Developer/FunctionalTesting/FunctionalTest.h"

#include "ScreenshotFunctionalTestBase.generated.h"

namespace ue5parse::runtime {

class UCameraComponent;

UCLASS()
class AScreenshotFunctionalTestBase : public AFunctionalTest {
    GENERATED_BODY()
public:
    explicit AScreenshotFunctionalTestBase(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UCameraComponent> ScreenshotCamera = nullptr;
};

}
