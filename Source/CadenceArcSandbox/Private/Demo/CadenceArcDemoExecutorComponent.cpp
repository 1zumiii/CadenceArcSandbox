#include "Demo/CadenceArcDemoExecutorComponent.h"
#include "CadenceArcDebugHelper.h"

void UCadenceArcDemoExecutorComponent::StartRequest(const FCadenceArcActionRequest& Request)
{
	if (BufferOpenDelay <= 0.0f || BufferOpenDelay > BufferCloseDelay || BufferCloseDelay > ActionDuration)
	{
		Debug::Print(FString::Printf(
			TEXT("Invalid timing configuration. BufferOpenDelay: %f, BufferCloseDelay: %f, ActionDuration: %f"),
			BufferOpenDelay,
			BufferCloseDelay,
			ActionDuration));
		return;
	}

	if (!IsValid(Resolver))
	{
		Debug::Print(TEXT("Resolver is not valid. Cannot start request."));
		return;
	}

	if (!IsValid(GetWorld()))
	{
		Debug::Print(TEXT("World is not valid. Cannot start request."));
		return;
	}

	ECadenceArcHandshakeResult HandshakeResult = Resolver->NotifyActionStarted(Request.RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Print(FString::Printf(
			TEXT("Failed to start action. Request Id: %s, Source: %s, Target: %s, Result: %s"),
			*FString::FromInt(Request.RequestId),
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
}

void UCadenceArcDemoExecutorComponent::HandleOpenBufferWindow(const int64 RequestId) const
{
	ECadenceArcHandshakeResult HandshakeResult = Resolver->OpenBufferWindow(RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Print(FString::Printf(
			TEXT("Failed to open buffer window. Request Id: %s, Result: %s"),
			*FString::FromInt(RequestId),
			*UEnum::GetValueAsString(HandshakeResult)));
	}
	else
	{
		Debug::Print(FString::Printf(
			TEXT("Buffer window opened. Request Id: %s"),
			*FString::FromInt(RequestId)));
	}
}

void UCadenceArcDemoExecutorComponent::HandleCloseBufferWindow(const int64 RequestId) const
{
	ECadenceArcHandshakeResult HandshakeResult = Resolver->CloseBufferWindow(RequestId);
	if (HandshakeResult != ECadenceArcHandshakeResult::Success)
	{
		Debug::Print(FString::Printf(
			TEXT("Failed to close buffer window. Request Id: %s, Result: %s"),
			*FString::FromInt(RequestId),
			*UEnum::GetValueAsString(HandshakeResult)));
	}
	else
	{
		Debug::Print(FString::Printf(
			TEXT("Buffer window closed. Request Id: %s"),
			*FString::FromInt(RequestId)));
	}
}

void UCadenceArcDemoExecutorComponent::HandleActionCompleted(const int64 RequestId)
{
	FCadenceArcActionCompletionOutcome Outcome = Resolver->NotifyActionCompleted(RequestId);
	// Consume the next action request if the handshake was successful and the buffer consume result is resolved
	if (
		Outcome.HandshakeResult == ECadenceArcHandshakeResult::Success &&
		Outcome.BufferConsumeResult == ECadenceArcBufferConsumeResult::Resolved
	)
	{
		StartRequest(Outcome.NextActionRequest);
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
	FCadenceArcActionRequest Request;
	switch (const ECadenceArcInputResult InputResult = Resolver->SubmitInput(InputTag, Request))
	{
	case ECadenceArcInputResult::Success:
		Debug::Print(FString::Printf(
			TEXT("Input submitted successfully. Request Id: %s, Source: %s, Target: %s"),
			*FString::FromInt(Request.RequestId),
			*Request.SourceActionTag.ToString(),
			*Request.TargetActionTag.ToString()));
		StartRequest(Request);
		break;
	case ECadenceArcInputResult::Buffered:
		Debug::Print(FString::Printf(TEXT("Input buffered: %s"), *InputTag.ToString()));
		break;
	default:
		Debug::Print(FString::Printf(TEXT("Input rejected: %s"), *UEnum::GetValueAsString(InputResult)));
		break;
	}
}

void UCadenceArcDemoExecutorComponent::ResetCombo()
{
	if (IsValid(Resolver) && Resolver->Reset())
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
	ECadenceArcResolverInitResult ResolverInitResult = Resolver->Initialize(ComboGraph);
	Debug::Print(FString::Printf(TEXT("Resolver Init Result: %s"), *UEnum::GetValueAsString(ResolverInitResult)));
	Debug::Print(FString::Printf(TEXT("Current Action Tag: %s"), *Resolver->GetCurrentActionTag().ToString()));
}
