#pragma once
#include "EnhancedInputComponent.h"
#include "Input/CadenceArcInputConfig.h"

namespace CadenceArc::Demo::Input
{
	template <
		typename UserClass,
		typename StartedCallback,
		typename CompletedCallback,
		typename CanceledCallback>
	void BindComboInputActions(
		UEnhancedInputComponent* InputComponent,
		const UCadenceArcInputConfig* InputConfig,
		UserClass* ContextObject,
		StartedCallback OnStarted,
		CompletedCallback OnCompleted,
		CanceledCallback OnCanceled)
	{
		check(InputComponent);
		check(InputConfig);
		check(ContextObject);
		
		TSet<FGameplayTag> UniqueTags;
		for (const FCadenceArcInputActionConfig& Config : InputConfig->ComboInputActions)
		{
			if (!Config.IsValid())
			{
				UE_LOG(
					LogTemp, Warning,
					TEXT("Invalid input action config: %s"),
					*Config.InputTag.ToString());
				continue;
			}
			// 检查ComboInputActions内是否存在重复的InputTag
			if (UniqueTags.Contains(Config.InputTag))
			{
				UE_LOG(
					LogTemp, Warning,
					TEXT("Duplicate input tag found in ComboInputActions: %s"),
					*Config.InputTag.ToString());
				continue;
			}
			UniqueTags.Add(Config.InputTag);

			InputComponent->BindAction(
				Config.InputAction,
				ETriggerEvent::Started,
				ContextObject,
				OnStarted,
				Config.InputTag,
				Config.InputMode);

			InputComponent->BindAction(
				Config.InputAction,
				ETriggerEvent::Completed,
				ContextObject,
				OnCompleted,
				Config.InputTag,
				Config.InputMode);

			InputComponent->BindAction(
				Config.InputAction,
				ETriggerEvent::Canceled,
				ContextObject,
				OnCanceled,
				Config.InputTag,
				Config.InputMode);
		}
	}
}
