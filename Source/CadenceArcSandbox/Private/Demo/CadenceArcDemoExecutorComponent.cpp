#include "Demo/CadenceArcDemoExecutorComponent.h"

#include "CadenceArcDebugHelper.h"
#include "Component/CadenceArcComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UCadenceArcDemoExecutorComponent::UCadenceArcDemoExecutorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCadenceArcDemoExecutorComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!bAutoExecute)
	{
		return;
	}
	CadenceArc = GetOwner() ? GetOwner()->FindComponentByClass<UCadenceArcComponent>() : nullptr;
	if (!IsValid(CadenceArc))
	{
		Debug::Print(TEXT("Demo executor needs a CadenceArc component on the same actor."), FColor::Red, 10.f);
		return;
	}
	CadenceArc->OnActionRequested.AddDynamic(this, &UCadenceArcDemoExecutorComponent::HandleActionRequested);
}

void UCadenceArcDemoExecutorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(CadenceArc))
	{
		CadenceArc->OnActionRequested.RemoveDynamic(this, &UCadenceArcDemoExecutorComponent::HandleActionRequested);
	}
	ClearExecutionTimers();
	Super::EndPlay(EndPlayReason);
}

void UCadenceArcDemoExecutorComponent::HandleActionRequested(const FCadenceArcActionRequest& Request)
{
	if (!IsValid(CadenceArc) || !IsValid(GetWorld()))
	{
		return;
	}

	if (BufferOpenDelay <= 0.0f || BufferOpenDelay > BufferCloseDelay || BufferCloseDelay > ActionDuration)
	{
		Debug::Print(FString::Printf(
			TEXT("Invalid timing configuration. BufferOpenDelay: %f, BufferCloseDelay: %f, ActionDuration: %f"),
			BufferOpenDelay, BufferCloseDelay, ActionDuration), FColor::Red, 5.f);
		CadenceArc->NotifyActionRejected(Request.RequestId);
		return;
	}

	// 调试场景：延迟开始。等待期间解析器停在 AwaitingStart，候选在 Arc Debugger 里显示为黄色
	if (StartDelaySeconds > 0.f)
	{
		ClearExecutionTimers();
		TWeakObjectPtr<UCadenceArcDemoExecutorComponent> WeakThis = this;
		GetWorld()->GetTimerManager().SetTimer(
			StartDelayTimerHandle,
			[WeakThis, Request]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->BeginExecution(Request);
				}
			},
			StartDelaySeconds,
			false);
		return;
	}
	BeginExecution(Request);
}

void UCadenceArcDemoExecutorComponent::BeginExecution(const FCadenceArcActionRequest& Request)
{
	if (!IsValid(CadenceArc) || !IsValid(GetWorld()))
	{
		return;
	}

	// 调试场景：执行器按节奏拒绝请求（例如资源不足、被硬直），已提交节点保持不变
	++RequestCounter;
	if (RejectEveryNthRequest > 0 && RequestCounter % RejectEveryNthRequest == 0)
	{
		Debug::Warn(FString::Printf(TEXT("Executor rejected request %lld (debug scenario)."), Request.RequestId));
		CadenceArc->NotifyActionRejected(Request.RequestId);
		return;
	}

	const ECadenceArcHandshakeResult HandshakeResult = CadenceArc->NotifyActionStarted(Request.RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Warn(FString::Printf(
			TEXT("Failed to start action. Request Id: %lld, Source: %s, Target: %s, Result: %s"),
			Request.RequestId, *Request.SourceActionTag.ToString(), *Request.TargetActionTag.ToString(),
			*UEnum::GetValueAsString(HandshakeResult)));
		return;
	}

	// 每个计时器都带着这次请求的编号：旧动作的计时器即使晚到，也会被解析器按过期回调拒绝
	ClearExecutionTimers();
	TWeakObjectPtr<UCadenceArcDemoExecutorComponent> WeakThis = this;
	const int64 RequestId = Request.RequestId;
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.SetTimer(BufferOpenTimerHandle, [WeakThis, RequestId]()
	{
		if (WeakThis.IsValid()) { WeakThis->HandleOpenBufferWindow(RequestId); }
	}, BufferOpenDelay * TimeScale, false);
	Timers.SetTimer(BufferCloseTimerHandle, [WeakThis, RequestId]()
	{
		if (WeakThis.IsValid()) { WeakThis->HandleCloseBufferWindow(RequestId); }
	}, BufferCloseDelay * TimeScale, false);
	Timers.SetTimer(ActionCompleteTimerHandle, [WeakThis, RequestId]()
	{
		if (WeakThis.IsValid()) { WeakThis->HandleActionCompleted(RequestId); }
	}, ActionDuration * TimeScale, false);
}

void UCadenceArcDemoExecutorComponent::HandleOpenBufferWindow(const int64 RequestId) const
{
	const ECadenceArcHandshakeResult HandshakeResult = CadenceArc->OpenBufferWindow(RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Warn(FString::Printf(TEXT("Failed to open buffer window. Request Id: %lld, Result: %s"),
		                            RequestId, *UEnum::GetValueAsString(HandshakeResult)));
	}
}

void UCadenceArcDemoExecutorComponent::HandleCloseBufferWindow(const int64 RequestId) const
{
	const ECadenceArcHandshakeResult HandshakeResult = CadenceArc->CloseBufferWindow(RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Warn(FString::Printf(TEXT("Failed to close buffer window. Request Id: %lld, Result: %s"),
		                            RequestId, *UEnum::GetValueAsString(HandshakeResult)));
	}
}

void UCadenceArcDemoExecutorComponent::HandleActionCompleted(const int64 RequestId)
{
	// 消费缓冲产生的下一个请求由 CadenceArc 组件从 OnActionRequested 发出，回到 HandleActionRequested
	const FCadenceArcActionCompletionOutcome Outcome = CadenceArc->NotifyActionCompleted(RequestId);
	if (Outcome.GetHandshakeResult() != ECadenceArcHandshakeResult::Success)
	{
		Debug::Warn(FString::Printf(TEXT("Action completion handshake failed. Request Id: %lld, Result: %s"),
		                            RequestId, *UEnum::GetValueAsString(Outcome.GetHandshakeResult())));
		return;
	}

	// 调试场景：同一个请求再报一次完成。解析器按过期回调拒绝它，不改变任何状态，
	// 只在 Arc History 里留下一条失败记录
	if (bSendStaleCallbacks)
	{
		CadenceArc->NotifyActionCompleted(RequestId);
	}
}

void UCadenceArcDemoExecutorComponent::ClearExecutionTimers()
{
	if (const UWorld* World = GetWorld())
	{
		FTimerManager& Timers = World->GetTimerManager();
		Timers.ClearTimer(StartDelayTimerHandle);
		Timers.ClearTimer(BufferOpenTimerHandle);
		Timers.ClearTimer(BufferCloseTimerHandle);
		Timers.ClearTimer(ActionCompleteTimerHandle);
	}
}
