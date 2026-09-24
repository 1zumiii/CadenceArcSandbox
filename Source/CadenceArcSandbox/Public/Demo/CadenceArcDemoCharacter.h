#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CadenceArcDemoCharacter.generated.h"

enum class ECadenceArcInputMode : uint8;
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="CadenceArc|Demo", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCadenceArcDemoExecutorComponent> DemoExecutor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Demo", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCadenceArcInputConfig> InputConfig;
	
	void Input_CadenceArcStarted(
		FGameplayTag InputTag, ECadenceArcInputMode Mode);

	void Input_CadenceArcCompleted(
		FGameplayTag InputTag, ECadenceArcInputMode Mode);

	void Input_CadenceArcCanceled(
		FGameplayTag InputTag, ECadenceArcInputMode Mode);
	void Input_ResetCombo();
};
