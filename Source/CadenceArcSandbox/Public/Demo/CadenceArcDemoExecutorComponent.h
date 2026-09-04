#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Graph/CadenceArcGraph.h"
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
	void SubmitInput(const FGameplayTag& InputTag);

	UFUNCTION(BlueprintCallable, Category="CadenceArc|Demo")
	void ResetCombo();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
