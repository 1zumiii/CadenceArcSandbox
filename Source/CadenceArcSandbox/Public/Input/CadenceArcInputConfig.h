#pragma once

#include "CoreMinimal.h"
#include "CadenceArcInputActionSet.h"
#include "CadenceArcInputConfig.generated.h"


class UInputMappingContext;
class UInputAction;

// Demo 的输入配置：连招输入映射来自 UCadenceArcInputActionSet（InputActions），这里补充 Mapping Context、重置和移动
UCLASS()
class CADENCEARCSANDBOX_API UCadenceArcInputConfig : public UCadenceArcInputActionSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Input")
	TObjectPtr<UInputAction> ResetInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Input")
	TObjectPtr<UInputAction> MoveInputAction;
};
