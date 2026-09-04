#pragma once
#include "EnhancedInputComponent.h"
#include "Input/CadenceArcInputConfig.h"

namespace CadenceArc::Demo::Input
{
	template <typename UserObject, typename CallbackFunc>
	void BindComboInputActions(
		UEnhancedInputComponent* InputComponent,
		const UCadenceArcInputConfig* InputConfig,
		UserObject* ContextObject,
		CallbackFunc Callback)
	{
		checkf(
			InputComponent,
			TEXT("Enhanced Input Component is null")
		);

		checkf(
			InputConfig,
			TEXT("CadenceArc Input Config is null")
		);
		for (const FCadenceArcInputActionConfig& ActionConfig : InputConfig->ComboInputActions)
		{
			if (!ActionConfig.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("Invalid input action config: %s"), *ActionConfig.InputTag.ToString());
				continue;
			}

			InputComponent->BindAction(
				ActionConfig.InputAction,
				ETriggerEvent::Started,
				ContextObject,
				Callback,
				ActionConfig.InputTag
			);
		}
	}
}
