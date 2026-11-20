#pragma once

#include "Engine/Source/Developer/FunctionalTesting/ScreenshotFunctionalTestBase.h"

#include "ScreenshotFunctionalTest.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AScreenshotFunctionalTest : public AScreenshotFunctionalTestBase {
    GENERATED_BODY()
public:
    explicit AScreenshotFunctionalTest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
