#pragma once
#include "CoreMinimal.h"
#include "Input/CadenceArcInputTypes.h"

struct FCadenceArcActionRequest;
struct FGameplayTag;
class UCadenceArcResolver;
class FCadenceArcInputTracker;

class CADENCEARCSANDBOX_API FCadenceArcHoldInputRouter
{
public:
	// 必须同步执行：Advance 产生的请求要在处理本次输入之前变成 Executing
	using FStartRequest = TFunctionRef<void(const FCadenceArcActionRequest&)>;

	explicit FCadenceArcHoldInputRouter(UCadenceArcResolver* InResolver);

	void Advance(const double Now, const FStartRequest& StartRequest);
	void Press(const FGameplayTag& Tag, const ECadenceArcInputMode Mode,
	           const double Now, const FStartRequest& StartRequest);
	void Release(const FGameplayTag& Tag, double Now, FStartRequest StartRequest);
	void Cancel(const FGameplayTag& Tag);
	void CancelAll(); // 失焦／解绑／EndPlay，不生成 Released

	bool IsTracking(const FGameplayTag& Tag) const;

	~FCadenceArcHoldInputRouter();

private:
	struct FTrackedPress
	{
		FCadenceArcInputToken Token;
		ECadenceArcInputMode Mode = ECadenceArcInputMode::PressOnly;
	};

	TWeakObjectPtr<UCadenceArcResolver> Resolver;
	TUniquePtr<FCadenceArcInputTracker> Tracker; // 不可复制；重建 = 换 Session
	TMap<FGameplayTag, FTrackedPress> PressedByTag;

	bool AdvanceAndDispatch(const double Now, const FStartRequest& StartRequest);
};
