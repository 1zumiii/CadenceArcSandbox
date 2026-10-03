#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Component/CadenceArcInputContextProvider.h"
#include "CadenceArcDemoCharacter.generated.h"

struct FGameplayTag;
struct FGameplayTagContainer;
struct FInputActionValue;
class UCadenceArcInputConfig;
class UCadenceArcComponent;
class UCadenceArcInputBinderComponent;
class UCadenceArcDemoExecutorComponent;

UCLASS()
class CADENCEARCSANDBOX_API ACadenceArcDemoCharacter : public ACharacter, public ICadenceArcInputContextProvider
{
	GENERATED_BODY()

public:
	ACadenceArcDemoCharacter();

protected:
	virtual void PawnClientRestart() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 按键时的方向作为事件上下文：CadenceArc 组件在按下和松开时调用
	virtual FGameplayTagContainer CollectInputContext_Implementation(FGameplayTag InputTag, ECadenceArcInputPhase Phase) const override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="CadenceArc|Demo", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCadenceArcDemoExecutorComponent> DemoExecutor;

	// CadenceArc 的标准入口：负责时间、逐帧推进、按键配对和请求出口。连招图配置在这个组件上
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="CadenceArc|Demo", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCadenceArcComponent> CadenceArcComponent;

	// 把 InputConfig 中的连招 Input Action 绑定到 CadenceArc 组件：Started 按下、Completed 松开、Canceled 取消
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="CadenceArc|Demo", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCadenceArcInputBinderComponent> CadenceArcInputBinder;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CadenceArc|Demo", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCadenceArcInputConfig> InputConfig;

	// 镜头参照的移动输入：Y 为前后，X 为左右；不依赖角色转向。
	FVector2D LastMoveAxis = FVector2D::ZeroVector;
	void Input_Move(const FInputActionValue& Value);
	void Input_MoveCompleted(const FInputActionValue& Value);
	FGameplayTagContainer MakeInputContextTags() const;
	void Input_ResetCombo();
};
