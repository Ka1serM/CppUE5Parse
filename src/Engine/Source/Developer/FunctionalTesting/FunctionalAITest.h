#pragma once

#include "Engine/Source/Developer/FunctionalTesting/FunctionalTest.h"

#include "FunctionalAITest.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AFunctionalAITestBase : public AFunctionalTest {
    GENERATED_BODY()
public:
    explicit AFunctionalAITestBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class AFunctionalAITest : public AFunctionalAITestBase {
    GENERATED_BODY()
public:
    explicit AFunctionalAITest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
