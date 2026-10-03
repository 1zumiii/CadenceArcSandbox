#include "Demo/CadenceArcDemoCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Math/RotationMatrix.h"
#include "Component/CadenceArcComponent.h"
#include "CadenceArcInputBinderComponent.h"
#include "Demo/CadenceArcDemoExecutorComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Input/CadenceArcInputConfig.h"

ACadenceArcDemoCharacter::ACadenceArcDemoCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	CadenceArcComponent = CreateDefaultSubobject<UCadenceArcComponent>(TEXT("CadenceArc"));
	CadenceArcInputBinder = CreateDefaultSubobject<UCadenceArcInputBinderComponent>(TEXT("CadenceArcInputBinder"));
	DemoExecutor = CreateDefaultSubobject<UCadenceArcDemoExecutorComponent>(TEXT("DemoExecutor"));
}

void ACadenceArcDemoCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	LastMoveAxis = FVector2D::ZeroVector;

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

	if (InputConfig->MoveInputAction)
	{
		EnhancedInputComponent->BindAction(InputConfig->MoveInputAction, ETriggerEvent::Triggered,
			this, &ACadenceArcDemoCharacter::Input_Move);
		EnhancedInputComponent->BindAction(InputConfig->MoveInputAction, ETriggerEvent::Completed,
			this, &ACadenceArcDemoCharacter::Input_MoveCompleted);
		EnhancedInputComponent->BindAction(InputConfig->MoveInputAction, ETriggerEvent::Canceled,
			this, &ACadenceArcDemoCharacter::Input_MoveCompleted);
	}

	// 输入方式只在输入配置里维护一份，绑定时写入 CadenceArc 组件；上下文由 CollectInputContext 提供
	CadenceArcInputBinder->BindInputActions(EnhancedInputComponent, InputConfig);

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

void ACadenceArcDemoCharacter::Input_Move(const FInputActionValue& Value)
{
	LastMoveAxis = Value.Get<FVector2D>();
	if (Controller)
	{
		const FRotator YawRotation(0.0, Controller->GetControlRotation().Yaw, 0.0);
		const FRotationMatrix Rotation(YawRotation);
		AddMovementInput(Rotation.GetUnitAxis(EAxis::X), LastMoveAxis.Y);
		AddMovementInput(Rotation.GetUnitAxis(EAxis::Y), LastMoveAxis.X);
	}
}

void ACadenceArcDemoCharacter::Input_MoveCompleted(const FInputActionValue& Value)
{
	LastMoveAxis = FVector2D::ZeroVector;
}

FGameplayTagContainer ACadenceArcDemoCharacter::CollectInputContext_Implementation(
	FGameplayTag InputTag, ECadenceArcInputPhase Phase) const
{
	return MakeInputContextTags();
}

FGameplayTagContainer ACadenceArcDemoCharacter::MakeInputContextTags() const
{
	FGameplayTagContainer ContextTags;
	// Demo 的 Forward 表示镜头参照下按着前；角色转身不会把 S/A/D 变成前输入。
	if (LastMoveAxis.Y > 0.7)
	{
		ContextTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("CadenceArc.Test.Context.Dir.Forward")));
	}
	return ContextTags;
}

void ACadenceArcDemoCharacter::Input_ResetCombo()
{
	if (IsValid(CadenceArcComponent))
	{
		CadenceArcComponent->ResetCombo(); // 结果记在 Arc History 里
	}
}
