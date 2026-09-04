#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CadenceArcInputConfig.generated.h"

class UInputMappingContext;
class UInputAction;

USTRUCT(BlueprintType)
struct CADENCEARCSANDBOX_API FCadenceArcInputActionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Input",
		meta=(Categories="CadenceArc.Test.Input"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Input")
	TObjectPtr<UInputAction> InputAction;

	bool IsValid() const
	{
		return InputTag.IsValid() && InputAction != nullptr;
	}
};

UCLASS()
class CADENCEARCSANDBOX_API UCadenceArcInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Input", meta=(TitleProperty="InputTag"))
	TArray<FCadenceArcInputActionConfig> ComboInputActions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Input")
	TObjectPtr<UInputAction> ResetInputAction;
};
