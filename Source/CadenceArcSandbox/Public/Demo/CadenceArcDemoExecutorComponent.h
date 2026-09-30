#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Graph/CadenceArcGraph.h"
#include "Input/CadenceArcHoldInputRouter.h"
#include "Resolver/CadenceArcResolver.h"
#include "CadenceArcDemoExecutorComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CADENCEARCSANDBOX_API UCadenceArcDemoExecutorComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo")
	TObjectPtr<UCadenceArcGraph> ComboGraph;

	UPROPERTY(Transient)
	TObjectPtr<UCadenceArcResolver> Resolver;

	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo|Timing", meta=(ClampMin="0.01", UIMin="0.01"))
	float BufferOpenDelay = 0.25f;

	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo|Timing", meta=(ClampMin="0.01", UIMin="0.01"))
	float BufferCloseDelay = 0.85f;

	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo|Timing", meta=(ClampMin="0.01", UIMin="0.01"))
	float ActionDuration = 1.20f;

	// ---- 调试场景（Phase 7 / P7-10）：只改变 Demo 执行器的行为，用来在 PIE 里稳定复现各种情况 ----

	// 动作时序的整体倍率：窗口开关和动作时长同比放大。调大后缓冲窗口足够长，可以从容地测试窗口内外的输入
	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo|Debug Scenarios", meta=(ClampMin="0.1", UIMin="0.1", UIMax="5"))
	float TimeScale = 1.f;

	// 请求产生后等待多久才真正开始执行。等待期间能在 Arc Debugger 里看到黄色的候选；
	// 这段时间里的新输入会因为上一个请求还没开始（RequestPending）而被忽略
	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo|Debug Scenarios", meta=(ClampMin="0", UIMin="0", UIMax="3"))
	float StartDelaySeconds = 0.f;

	// 每第 N 个请求由执行器拒绝（0 表示从不）。被拒绝时已提交节点保持不变
	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo|Debug Scenarios", meta=(ClampMin="0", UIMin="0", UIMax="5"))
	int32 RejectEveryNthRequest = 0;

	// 每次动作完成后再用同一个请求号发一次 Completed，演示过期回调被 Resolver 拒绝且不影响状态
	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo|Debug Scenarios")
	bool bSendStaleCallbacks = false;

	int32 RequestCounter = 0; // 用于 RejectEveryNthRequest

	FTimerHandle StartDelayTimerHandle;
	FTimerHandle BufferOpenTimerHandle;
	FTimerHandle BufferCloseTimerHandle;
	FTimerHandle ActionCompleteTimerHandle;

	TUniquePtr<FCadenceArcHoldInputRouter> InputRouter;

	// Helper Functions
	void StartRequest(const FCadenceArcActionRequest& Request);
	void BeginExecution(const FCadenceArcActionRequest& Request);
	void HandleOpenBufferWindow(const int64 RequestId) const;
	void HandleCloseBufferWindow(const int64 RequestId) const;
	void HandleActionCompleted(const int64 RequestId);
	void ClearExecutionTimers();

public:
	// Sets default values for this component's properties
	UCadenceArcDemoExecutorComponent();

	UFUNCTION(BlueprintCallable, Category="CadenceArc|Demo")
	void ResetCombo();
	
	UFUNCTION(BlueprintCallable, Category="CadenceArc|Demo")
	void PressInput(
		const FGameplayTag& InputTag,
		ECadenceArcInputMode Mode
	);
	UFUNCTION(BlueprintCallable, Category="CadenceArc|Demo")
	void ReleaseInput(const FGameplayTag& InputTag);
	UFUNCTION(BlueprintCallable, Category="CadenceArc|Demo")
	void CancelInput(const FGameplayTag& InputTag);

	virtual void TickComponent(
		float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
};
