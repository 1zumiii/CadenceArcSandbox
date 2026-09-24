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

		for (const FCadenceArcInputActionConfig& Config
		     : InputConfig->ComboInputActions)
		{
			if (!Config.IsValid())
			{
				UE_LOG(
					LogTemp, Warning,
					TEXT("Invalid input action config: %s"),
					*Config.InputTag.ToString());
				continue;
			}

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
