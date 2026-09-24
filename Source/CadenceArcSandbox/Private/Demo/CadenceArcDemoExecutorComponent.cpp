#include "Demo/CadenceArcDemoExecutorComponent.h"
#include "CadenceArcDebugHelper.h"

void UCadenceArcDemoExecutorComponent::StartRequest(const FCadenceArcActionRequest& Request)
{
	if (!IsValid(Resolver))
	{
		Debug::Print(TEXT("Resolver is not valid. Cannot start request."));
		return;
	}

	if (BufferOpenDelay <= 0.0f || BufferOpenDelay > BufferCloseDelay || BufferCloseDelay > ActionDuration)
	{
		Debug::Print(FString::Printf(
			TEXT("Invalid timing configuration. BufferOpenDelay: %f, BufferCloseDelay: %f, ActionDuration: %f"),
			BufferOpenDelay,
			BufferCloseDelay,
			ActionDuration));
		Resolver->NotifyActionRejected(Request.RequestId);
		return;
	}

	if (!IsValid(GetWorld()))
	{
		Debug::Print(TEXT("World is not valid. Cannot start request."));
		Resolver->NotifyActionRejected(Request.RequestId);
		return;
	}

	const ECadenceArcHandshakeResult HandshakeResult = Resolver->NotifyActionStarted(Request.RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Print(FString::Printf(
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
		BufferOpenDelay,
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
		BufferCloseDelay,
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
		ActionDuration,
		false
	);
	Debug::Print(FString::Printf(
		TEXT("Action started. Request Id: %lld, Source: %s, Target: %s"),
		Request.RequestId,
		*Request.SourceActionTag.ToString(),
		*Request.TargetActionTag.ToString()
	));
}

void UCadenceArcDemoExecutorComponent::HandleOpenBufferWindow(const int64 RequestId) const
{
	ECadenceArcHandshakeResult HandshakeResult = Resolver->OpenBufferWindow(RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Print(FString::Printf(
			TEXT("Failed to open buffer window. Request Id: %lld, Result: %s"),
			RequestId, *UEnum::GetValueAsString(HandshakeResult)));
	}
	else
	{
		Debug::Print(FString::Printf(
			TEXT("Buffer window opened. Request Id: %lld"), RequestId));
	}
}

void UCadenceArcDemoExecutorComponent::HandleCloseBufferWindow(const int64 RequestId) const
{
	const ECadenceArcHandshakeResult HandshakeResult = Resolver->CloseBufferWindow(RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Print(FString::Printf(
			TEXT("Failed to close buffer window. Request Id: %lld, Result: %s"),
			RequestId, *UEnum::GetValueAsString(HandshakeResult)));
	}
	else
	{
		Debug::Print(FString::Printf(
			TEXT("Buffer window closed. Request Id: %lld"), RequestId));
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
		Debug::Print(FString::Printf(
			TEXT("Action completion handshake failed, Consuming not attempted. Request Id: %lld, Result: %s"),
			RequestId, *UEnum::GetValueAsString(Outcome.GetHandshakeResult())));
		return;
	}

	Debug::Print(FString::Printf(
		TEXT("Action completed successfully. Request Id: "
			"%lld, Completion time: %f, Handshake Result: %s, Resolution Category: %s, Resolution Reason: %s"
		),
		RequestId,
		GetWorld()->GetTimeSeconds(),
		*UEnum::GetValueAsString(Outcome.GetHandshakeResult()),
		*UEnum::GetValueAsString(Outcome.GetBufferConsumption()),
		*UEnum::GetValueAsString(Outcome.GetBufferConsumptionReason())
	));
	// 后面的日志和 HasNextActionRequest 处理保持不变


	// Consume the next action request if the handshake was successful and the buffer consume result is resolved
	if (Outcome.HasNextActionRequest())
	{
		StartRequest(Outcome.GetNextActionRequest());
	}
}

void UCadenceArcDemoExecutorComponent::ClearExecutionTimers()
{
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
	if (IsValid(Resolver) && Resolver->Reset() == ECadenceArcResolverResetResult::Success)
	{
		Debug::Print(
			FString::Printf(TEXT("Combo reset. Current Action Tag: %s"), *Resolver->GetCurrentActionTag().ToString()));
	}
}

void UCadenceArcDemoExecutorComponent::PressInput(const FGameplayTag& InputTag, ECadenceArcInputMode Mode)
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
		}
	);
}

void UCadenceArcDemoExecutorComponent::ReleaseInput(const FGameplayTag& InputTag)
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
		}
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
	if (!IsValid(Resolver)) { return; }
	InputRouter->Advance(GetWorld()->GetTimeSeconds(),
	                     [this](const FCadenceArcActionRequest& R) { StartRequest(R); }
	);
}

void UCadenceArcDemoExecutorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	InputRouter->CancelAll();
	ClearExecutionTimers();
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
			TEXT("Failed to initialize Resolver. Result: %s"), *UEnum::GetValueAsString(ResolverInitResult)));
	}
	Debug::Print(FString::Printf(TEXT("Resolver Init Result: %s"), *UEnum::GetValueAsString(ResolverInitResult)));
	Debug::Print(FString::Printf(TEXT("Current Action Tag: %s"), *Resolver->GetCurrentActionTag().ToString()));
}
