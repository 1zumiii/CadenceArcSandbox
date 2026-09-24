#include "Input/CadenceArcHoldInputRouter.h"

#include "CadenceArcDebugHelper.h"
#include "Input/CadenceArcInputTracker.h"
#include "Resolver/CadenceArcResolver.h"

FCadenceArcHoldInputRouter::FCadenceArcHoldInputRouter(UCadenceArcResolver* InResolver)
{
	Resolver = InResolver;
	Tracker = MakeUnique<FCadenceArcInputTracker>();
}

void FCadenceArcHoldInputRouter::Advance(const double Now, const FStartRequest& StartRequest)
{
	AdvanceAndDispatch(Now, StartRequest);
}

void FCadenceArcHoldInputRouter::Press(
	const FGameplayTag& Tag, const ECadenceArcInputMode Mode,
	const double Now, const FStartRequest& StartRequest
)
{
	UCadenceArcResolver* R = Resolver.Get();
	if (!R) { return; }
	// 检查Mode的合法性
	if (Mode != ECadenceArcInputMode::HoldRelease && Mode != ECadenceArcInputMode::PressOnly)
	{
		return;
	}
	// 先推进，处理截至至Now已经到期的长按事件，如果没有调用，Resolver内部可能还仍然保存着旧的待松手资格
	if (!AdvanceAndDispatch(Now, StartRequest))
	{
		Debug::Print(TEXT("AdvanceAndDispatch failed during Press."));
	}
	const FCadenceArcInputTrackingOutcome Pressed = Tracker->Press(Tag, Now);
	if (Pressed.GetResult() != ECadenceArcInputTrackingResult::PressedProduced)
	{
		return;
	}
	PressedByTag.Add(Tag, FTrackedPress{Pressed.GetToken(), Mode});
	switch (Mode)
	{
	case ECadenceArcInputMode::PressOnly:
		{
			if (const FCadenceArcSubmitOutcome Outcome = R->SubmitInput(Pressed.GetInputEvent()); Outcome.
				HasActionRequest())
			{
				StartRequest(Outcome.GetActionRequest());
			}
			break;
		}

	case ECadenceArcInputMode::HoldRelease:
		R->BeginInputHold(Pressed.GetToken(), Pressed.GetInputEvent());
		break;
	}
}

void FCadenceArcHoldInputRouter::Release(const FGameplayTag& Tag, double Now, FStartRequest StartRequest)
{
	const UCadenceArcResolver* R = Resolver.Get();
	if (!R) { return; }
	// 不能Advance被拒绝就return，否则如果某次Release被丢，会导致这个键在Tracker内一直处于被按住状态
	// 之后每次 Press 都返回 AlreadyPressed
	if (!AdvanceAndDispatch(Now, StartRequest))
	{
		Debug::Print(TEXT("AdvanceAndDispatch failed during Release."));
	}
	FTrackedPress* FoundPress = PressedByTag.Find(Tag);
	if (!FoundPress) { return; }
	const FTrackedPress Press = *FoundPress;
	FCadenceArcInputTrackingOutcome ReleaseOutcome = Tracker->Release(Press.Token, Now);
	if (ReleaseOutcome.GetResult() != ECadenceArcInputTrackingResult::ReleasedProduced)
	{
		return;
	}
	PressedByTag.Remove(Tag);
	switch (Press.Mode)
	{
	case ECadenceArcInputMode::PressOnly:
		break;
	case ECadenceArcInputMode::HoldRelease:
		FCadenceArcInputAdvanceOutcome Outcome = Resolver->ReleaseInputHold(
			Press.Token, ReleaseOutcome.GetInputEvent()
		);
		if (Outcome.HasActionRequest())
		{
			StartRequest(Outcome.GetResolution().GetActionRequest());
		}
		break;
	}
}

void FCadenceArcHoldInputRouter::Cancel(const FGameplayTag& Tag)
{
	if (!Resolver.IsValid()) { return; }
	const FTrackedPress* Found = PressedByTag.Find(Tag);
	if (!Found) { return; }
	Resolver->CancelInputHold(Found->Token);
	Tracker->Cancel(Found->Token);
	PressedByTag.Remove(Tag);
}

void FCadenceArcHoldInputRouter::CancelAll()
{
	if (Resolver.IsValid())
	{
		for (const TTuple<FGameplayTag, FTrackedPress>& Pair : PressedByTag)
		{
			const FTrackedPress Press = Pair.Value;
			Resolver->CancelInputHold(Press.Token);
		}
	}
	Tracker->ClearAll();
	PressedByTag.Reset();
}

// 检查物理配对是否存在
bool FCadenceArcHoldInputRouter::IsTracking(const FGameplayTag& Tag) const
{
	return PressedByTag.Contains(Tag);
}

FCadenceArcHoldInputRouter::~FCadenceArcHoldInputRouter() = default;

// Press和Release都需要先推进
bool FCadenceArcHoldInputRouter::AdvanceAndDispatch(const double Now, const FStartRequest& StartRequest)
{
	UCadenceArcResolver* R = Resolver.Get();
	if (!R) { return false; }
	const FCadenceArcInputAdvanceOutcome Outcome = R->AdvanceInputTime(Now);
	if (!Outcome.IsAccepted()) { return false; }
	if (Outcome.HasActionRequest())
	{
		StartRequest(Outcome.GetResolution().GetActionRequest());
	}
	return Resolver.IsValid();
}
