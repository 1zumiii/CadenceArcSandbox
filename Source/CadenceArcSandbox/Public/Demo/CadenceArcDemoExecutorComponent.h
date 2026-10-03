#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Resolver/CadenceArcResolverTypes.h"
#include "CadenceArcDemoExecutorComponent.generated.h"

class UCadenceArcComponent;

/**
 * 基于 Timer 的演示执行器：订阅同一个 Actor 上 UCadenceArcComponent 的 OnActionRequested，
 * 用固定时长模拟动作，并在对应时刻回调开始、缓冲窗口开关和完成。只负责执行，输入和时间都由 CadenceArc 组件处理。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CADENCEARCSANDBOX_API UCadenceArcDemoExecutorComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo|Timing", meta=(ClampMin="0.01", UIMin="0.01"))
	float BufferOpenDelay = 0.25f;

	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo|Timing", meta=(ClampMin="0.01", UIMin="0.01"))
	float BufferCloseDelay = 0.85f;

	// 动作时长。到时调用 NotifyActionCompleted，停顿从这个时刻开始计算
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

	// 每次动作完成后再用同一个请求号发一次 Completed，演示过期回调被拒绝且不影响状态
	UPROPERTY(EditAnywhere, Category="CadenceArc|Demo|Debug Scenarios")
	bool bSendStaleCallbacks = false;

	UPROPERTY(Transient)
	TObjectPtr<UCadenceArcComponent> CadenceArc;

	int32 RequestCounter = 0; // 用于 RejectEveryNthRequest

	FTimerHandle StartDelayTimerHandle;
	FTimerHandle BufferOpenTimerHandle;
	FTimerHandle BufferCloseTimerHandle;
	FTimerHandle ActionCompleteTimerHandle;

	UFUNCTION()
	void HandleActionRequested(const FCadenceArcActionRequest& Request);
	void BeginExecution(const FCadenceArcActionRequest& Request);
	void HandleOpenBufferWindow(int64 RequestId) const;
	void HandleCloseBufferWindow(int64 RequestId) const;
	void HandleActionCompleted(int64 RequestId);
	void ClearExecutionTimers();

public:
	// 关闭后不订阅动作请求。BP_CadenceArcBlueprintDemo 继承 Demo 角色，由蓝图自己处理请求，因此关闭此项。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CadenceArc|Demo")
	bool bAutoExecute = true;

	UCadenceArcDemoExecutorComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;
};
