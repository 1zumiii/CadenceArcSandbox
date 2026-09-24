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

	FTimerHandle BufferOpenTimerHandle;
	FTimerHandle BufferCloseTimerHandle;
	FTimerHandle ActionCompleteTimerHandle;

	TUniquePtr<FCadenceArcHoldInputRouter> InputRouter;

	// Helper Functions
	void StartRequest(const FCadenceArcActionRequest& Request);
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
