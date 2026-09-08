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
	FCadenceArcActionCompletionOutcome Outcome = Resolver->NotifyActionCompleted(
		RequestId, GetWorld()->GetTimeSeconds()
	);
	Debug::Print(FString::Printf(
		TEXT("Completion callback result. Request Id: "
			"%lld, Completion time: %f, Handshake Result: %s, Resolution Category: %s, Resolution Reason: %s"
		),
		RequestId,
		GetWorld()->GetTimeSeconds(),
		*UEnum::GetValueAsString(Outcome.GetHandshakeResult()),
		*UEnum::GetValueAsString(Outcome.GetBufferConsumption()),
		*UEnum::GetValueAsString(Outcome.GetBufferConsumptionReason())
	));

	if (Outcome.GetHandshakeResult() != ECadenceArcHandshakeResult::Success)
	{
		Debug::Print(FString::Printf(
			TEXT("Action completion handshake failed. Request Id: %lld, Result: %s"),
			RequestId, *UEnum::GetValueAsString(Outcome.GetHandshakeResult())));
		return;
	}

	Debug::Print(FString::Printf(
		TEXT("Action completed successfully. Request Id: %lld"),
		RequestId
	));

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
	PrimaryComponentTick.bCanEverTick = false;
}

void UCadenceArcDemoExecutorComponent::SubmitInput(const FGameplayTag& InputTag)
{
	if (!IsValid(Resolver))
	{
		Debug::Print(TEXT("Resolver is not valid. Cannot submit input."));
		return;
	}
	const FCadenceArcInputEvent InputEvent = {.InputTag = InputTag, .TimestampSeconds = GetWorld()->GetTimeSeconds()};
	const FCadenceArcSubmitOutcome SubmitOutcome = Resolver->SubmitInput(InputEvent);
	Debug::Print(FString::Printf(TEXT("Request Produced for Input: %s"), *InputTag.ToString()));
	if (SubmitOutcome.HasActionRequest())
	{
		StartRequest(SubmitOutcome.GetActionRequest());
	}
	switch (SubmitOutcome.GetCategory())
	{
	case ECadenceArcResolutionCategory::RequestProduced:
		Debug::Print(FString::Printf(
			TEXT("Request Produced for Input: %s"), *InputTag.ToString()));
		break;
	case ECadenceArcResolutionCategory::Buffered:
		Debug::Print(FString::Printf(TEXT("Input buffered: %s"), *InputTag.ToString()));
		break;
	case ECadenceArcResolutionCategory::NoAction:
		Debug::Print(FString::Printf(
				TEXT("Input ignored: %s, Reason: %s"), *InputTag.ToString(),
				*UEnum::GetValueAsString(SubmitOutcome.GetReason()))
		);
		break;
	default:
		Debug::Print(FString::Printf(TEXT("Input rejected: %s"), *UEnum::GetValueAsString(SubmitOutcome.GetReason())));
		break;
	}
}

void UCadenceArcDemoExecutorComponent::ResetCombo()
{
	if (IsValid(Resolver) && Resolver->Reset() == ECadenceArcResolverResetResult::Success)
	{
		Debug::Print(
			FString::Printf(TEXT("Combo reset. Current Action Tag: %s"), *Resolver->GetCurrentActionTag().ToString()));
	}
}


// Called when the game starts
void UCadenceArcDemoExecutorComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!IsValid(ComboGraph)) { return; }
	Resolver = NewObject<UCadenceArcResolver>(this);
	const ECadenceArcResolverInitResult ResolverInitResult = Resolver->Initialize(ComboGraph);
	Debug::Print(FString::Printf(TEXT("Resolver Init Result: %s"), *UEnum::GetValueAsString(ResolverInitResult)));
	Debug::Print(FString::Printf(TEXT("Current Action Tag: %s"), *Resolver->GetCurrentActionTag().ToString()));
}
