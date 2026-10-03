#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CadenceArcDemoCharacter.generated.h"

enum class ECadenceArcInputMode : uint8;
struct FGameplayTag;
struct FGameplayTagContainer;
struct FInputActionValue;
class UCadenceArcInputConfig;
class UCadenceArcComponent;
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

	// CadenceArc 的标准入口：负责时间、逐帧推进、按键配对和请求出口。连招图配置在这个组件上
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="CadenceArc|Demo", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCadenceArcComponent> CadenceArcComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Demo", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCadenceArcInputConfig> InputConfig;

	// 镜头参照的移动输入：Y 为前后，X 为左右；不依赖角色转向。
	FVector2D LastMoveAxis = FVector2D::ZeroVector;
	void Input_Move(const FInputActionValue& Value);
	void Input_MoveCompleted(const FInputActionValue& Value);
	FGameplayTagContainer MakeInputContextTags() const;
	
	void Input_CadenceArcStarted(
		FGameplayTag InputTag, ECadenceArcInputMode Mode);

	void Input_CadenceArcCompleted(
		FGameplayTag InputTag, ECadenceArcInputMode Mode);

	void Input_CadenceArcCanceled(
		FGameplayTag InputTag, ECadenceArcInputMode Mode);
	void Input_ResetCombo();
};
