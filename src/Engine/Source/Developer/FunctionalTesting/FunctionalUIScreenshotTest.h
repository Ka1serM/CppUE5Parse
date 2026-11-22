#pragma once

#include "Engine/Source/Developer/FunctionalTesting/ScreenshotFunctionalTestBase.h"

#include "FunctionalUIScreenshotTest.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AFunctionalUIScreenshotTest : public AScreenshotFunctionalTestBase {
    GENERATED_BODY()
public:
    explicit AFunctionalUIScreenshotTest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
