#pragma once

#include "Commandlets/Commandlet.h"
#include "RefGenCommandlet.generated.h"

UCLASS()
class URefGenCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URefGenCommandlet();
	virtual int32 Main(const FString& Params) override;
};
