#include "Demo/CadenceArcDemoCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Demo/CadenceArcDemoExecutorComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Input/CadenceArcInputBinding.h"
#include "Input/CadenceArcInputConfig.h"

ACadenceArcDemoCharacter::ACadenceArcDemoCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	DemoExecutor = CreateDefaultSubobject<UCadenceArcDemoExecutorComponent>(TEXT("DemoExecutor"));
}

void ACadenceArcDemoCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	if (!IsValid(InputConfig) || !IsValid(InputConfig->DefaultMappingContext))
	{
		return;
	}

	const APlayerController* PlayerController =
		Cast<APlayerController>(GetController());

	if (!PlayerController)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	if (!InputSubsystem)
	{
		return;
	}

	InputSubsystem->RemoveMappingContext(InputConfig->DefaultMappingContext);
	InputSubsystem->AddMappingContext(InputConfig->DefaultMappingContext, 0);
}


void ACadenceArcDemoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	checkf(IsValid(InputConfig), TEXT("InputConfig is not valid. Please assign a valid InputConfig in the editor."));

	CadenceArc::Demo::Input::BindComboInputActions(
		EnhancedInputComponent,
		InputConfig,
		this,
		&ACadenceArcDemoCharacter::Input_CadenceArcStarted,
		&ACadenceArcDemoCharacter::Input_CadenceArcCompleted,
		&ACadenceArcDemoCharacter::Input_CadenceArcCanceled
	);

	if (InputConfig->ResetInputAction)
	{
		EnhancedInputComponent->BindAction(
			InputConfig->ResetInputAction,
			ETriggerEvent::Started,
			this,
			&ACadenceArcDemoCharacter::Input_ResetCombo
		);
	}
}

void ACadenceArcDemoCharacter::Input_CadenceArcStarted(FGameplayTag InputTag, ECadenceArcInputMode Mode)
{
	if (IsValid(DemoExecutor))
	{
		DemoExecutor->PressInput(InputTag, Mode);
	}
}

void ACadenceArcDemoCharacter::Input_CadenceArcCompleted(FGameplayTag InputTag, ECadenceArcInputMode Mode)
{
	if (IsValid(DemoExecutor))
	{
		DemoExecutor->ReleaseInput(InputTag);
	}
}

void ACadenceArcDemoCharacter::Input_CadenceArcCanceled(FGameplayTag InputTag, ECadenceArcInputMode Mode)
{
	if (IsValid(DemoExecutor))
	{
		DemoExecutor->CancelInput(InputTag);
	}
}

void ACadenceArcDemoCharacter::Input_ResetCombo()
{
	if (IsValid(DemoExecutor))
	{
		DemoExecutor->ResetCombo();
	}
}
