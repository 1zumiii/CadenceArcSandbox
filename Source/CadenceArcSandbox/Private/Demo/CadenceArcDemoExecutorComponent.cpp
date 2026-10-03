#include "Demo/CadenceArcDemoExecutorComponent.h"
#include "CadenceArcDebugHelper.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UCadenceArcDemoExecutorComponent::StartRequest(const FCadenceArcActionRequest& Request)
{
	if (!IsValid(Resolver))
	{
		Debug::Print(TEXT("Resolver is not valid. Cannot start request."), FColor::Red, 5.f);
		return;
	}

	if (BufferOpenDelay <= 0.0f || BufferOpenDelay > BufferCloseDelay || BufferCloseDelay > ActionDuration)
	{
		Debug::Print(FString::Printf(
			TEXT("Invalid timing configuration. BufferOpenDelay: %f, BufferCloseDelay: %f, ActionDuration: %f"),
			BufferOpenDelay,
			BufferCloseDelay,
			ActionDuration), FColor::Red, 5.f);
		Resolver->NotifyActionRejected(Request.RequestId);
		return;
	}

	if (!IsValid(GetWorld()))
	{
		Debug::Print(TEXT("World is not valid. Cannot start request."), FColor::Red, 5.f);
		Resolver->NotifyActionRejected(Request.RequestId);
		return;
	}

	// 调试场景：延迟开始。等待期间 Resolver 停在 AwaitingStart，候选在 Arc Debugger 里显示为黄色
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
			false
		);
		return;
	}
	BeginExecution(Request);
}

void UCadenceArcDemoExecutorComponent::BeginExecution(const FCadenceArcActionRequest& Request)
{
	if (!IsValid(Resolver) || !IsValid(GetWorld()))
	{
		return;
	}

	// 调试场景：执行器按节奏拒绝请求（例如资源不足、被硬直），已提交节点保持不变
	++RequestCounter;
	if (RejectEveryNthRequest > 0 && RequestCounter % RejectEveryNthRequest == 0)
	{
		Debug::Warn(FString::Printf(TEXT("Executor rejected request %lld (debug scenario)."), Request.RequestId));
		Resolver->NotifyActionRejected(Request.RequestId);
		return;
	}

	const ECadenceArcHandshakeResult HandshakeResult = Resolver->NotifyActionStarted(Request.RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Warn(FString::Printf(
			TEXT("Failed to start action. Request Id: %lld, Source: %s, Target: %s, Result: %s"),
			Request.RequestId,
			*Request.SourceActionTag.ToString(),
			*Request.TargetActionTag.ToString(),
			*UEnum::GetValueAsString(HandshakeResult)));
		return;
	};

	ClearExecutionTimers();
	TWeakObjectPtr<UCadenceArcDemoExecutorComponent> WeakThis = this;
	int64 CurrentRequestId = Request.RequestId;
	GetWorld()->GetTimerManager().SetTimer(
		BufferOpenTimerHandle,
		[WeakThis, CurrentRequestId]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->HandleOpenBufferWindow(CurrentRequestId);
			}
		},
		BufferOpenDelay * TimeScale,
		false
	);
	GetWorld()->GetTimerManager().SetTimer(
		BufferCloseTimerHandle,
		[WeakThis, CurrentRequestId]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->HandleCloseBufferWindow(CurrentRequestId);
			}
		},
		BufferCloseDelay * TimeScale,
		false
	);
	GetWorld()->GetTimerManager().SetTimer(
		ActionCompleteTimerHandle,
		[WeakThis, CurrentRequestId]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->HandleActionCompleted(CurrentRequestId);
			}
		},
		ActionDuration * TimeScale,
		false
	);
}

void UCadenceArcDemoExecutorComponent::HandleOpenBufferWindow(const int64 RequestId) const
{
	ECadenceArcHandshakeResult HandshakeResult = Resolver->OpenBufferWindow(RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Warn(FString::Printf(
			TEXT("Failed to open buffer window. Request Id: %lld, Result: %s"),
			RequestId, *UEnum::GetValueAsString(HandshakeResult)));
	}
}

void UCadenceArcDemoExecutorComponent::HandleCloseBufferWindow(const int64 RequestId) const
{
	const ECadenceArcHandshakeResult HandshakeResult = Resolver->CloseBufferWindow(RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Warn(FString::Printf(
			TEXT("Failed to close buffer window. Request Id: %lld, Result: %s"),
			RequestId, *UEnum::GetValueAsString(HandshakeResult)));
	}
}

void UCadenceArcDemoExecutorComponent::HandleActionCompleted(const int64 RequestId)
{
	const double Now = GetWorld()->GetTimeSeconds();
	InputRouter->Advance(Now,
	                     [this](const FCadenceArcActionRequest& R) { StartRequest(R); }
	);
	const FCadenceArcActionCompletionOutcome Outcome = Resolver->NotifyActionCompleted(RequestId, Now);
	if (Outcome.GetHandshakeResult() != ECadenceArcHandshakeResult::Success)
	{
		Debug::Warn(FString::Printf(
			TEXT("Action completion handshake failed, Consuming not attempted. Request Id: %lld, Result: %s"),
			RequestId, *UEnum::GetValueAsString(Outcome.GetHandshakeResult())));
		return;
	}

	// Consume the next action request if the handshake was successful and the buffer consume result is resolved
	if (Outcome.HasNextActionRequest())
	{
		StartRequest(Outcome.GetNextActionRequest());
	}

	// 调试场景：同一个请求再报一次完成。Resolver 会按过期回调拒绝它，不改变任何状态，
	// 只在 Arc History 里留下一条失败记录
	if (bSendStaleCallbacks)
	{
		Resolver->NotifyActionCompleted(RequestId, GetWorld()->GetTimeSeconds());
	}
}

void UCadenceArcDemoExecutorComponent::ClearExecutionTimers()
{
	GetWorld()->GetTimerManager().ClearTimer(StartDelayTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(BufferOpenTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(BufferCloseTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ActionCompleteTimerHandle);
}

// Sets default values for this component's properties
UCadenceArcDemoExecutorComponent::UCadenceArcDemoExecutorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCadenceArcDemoExecutorComponent::ResetCombo()
{
	if (IsValid(Resolver))
	{
		Resolver->Reset(); // 结果记在 Arc History 里
	}
}

void UCadenceArcDemoExecutorComponent::PressInput(
	const FGameplayTag& InputTag, ECadenceArcInputMode Mode, const FGameplayTagContainer& ContextTags)
{
	UWorld* World = GetWorld();
	if (!InputRouter || !World)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	InputRouter->Press(
		InputTag, Mode, Now,
		[this](const FCadenceArcActionRequest& Request)
		{
			StartRequest(Request);
		}, ContextTags
	);
}

void UCadenceArcDemoExecutorComponent::ReleaseInput(
	const FGameplayTag& InputTag, const FGameplayTagContainer& ContextTags)
{
	UWorld* World = GetWorld();
	if (!InputRouter || !World)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	InputRouter->Release(
		InputTag, Now,
		[this](const FCadenceArcActionRequest& Request)
		{
			StartRequest(Request);
		}, ContextTags
	);
}

void UCadenceArcDemoExecutorComponent::CancelInput(const FGameplayTag& InputTag)
{
	UWorld* World = GetWorld();
	if (!InputRouter || !World)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	InputRouter->Cancel(InputTag);
}

void UCadenceArcDemoExecutorComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsValid(Resolver) || InputRouter == nullptr) { return; }
	InputRouter->Advance(GetWorld()->GetTimeSeconds(),
	                     [this](const FCadenceArcActionRequest& R) { StartRequest(R); }
	);
}

void UCadenceArcDemoExecutorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InputRouter == nullptr || !IsValid(Resolver))
	{
		Super::EndPlay(EndPlayReason);
		return;
	}
	InputRouter->CancelAll();
	ClearExecutionTimers();
	Super::EndPlay(EndPlayReason);
}


// Called when the game starts
void UCadenceArcDemoExecutorComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!IsValid(ComboGraph)) { return; }
	Resolver = NewObject<UCadenceArcResolver>(this);
	const ECadenceArcResolverInitResult ResolverInitResult = Resolver->Initialize(ComboGraph);
	if (ResolverInitResult == ECadenceArcResolverInitResult::Success)
	{
		InputRouter = MakeUnique<FCadenceArcHoldInputRouter>(Resolver);
	}
	else
	{
		Debug::Print(FString::Printf(
			TEXT("Failed to initialize Resolver. Result: %s"), *UEnum::GetValueAsString(ResolverInitResult)), FColor::Red, 10.f);
	}
}
