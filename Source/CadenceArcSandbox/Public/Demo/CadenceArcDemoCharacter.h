#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CadenceArcDemoCharacter.generated.h"

struct FGameplayTag;
class UCadenceArcInputConfig;
class UCadenceArcDemoExecutorComponent;

UCLASS()
class CADENCEARCSANDBOX_API ACadenceArcDemoCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACadenceArcDemoCharacter();

protected:
	virtual void PawnClientRestart() override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category="CadenceArc|Demo", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCadenceArcDemoExecutorComponent> DemoExecutor;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category="CadenceArc|Demo", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCadenceArcInputConfig> InputConfig;
	
	void Input_CadenceArcAction(const FGameplayTag InputTag);
	void Input_ResetCombo();
};
