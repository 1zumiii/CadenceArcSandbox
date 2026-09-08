#pragma once
#include "CadenceArcSandbox/CadenceArcSandbox.h"

namespace Debug
{
	static void Print(const FString& Message, const FColor& Color = FColor::MakeRandomColor(), float Duration = 2.0f)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
		}
		UE_LOG(LogCadenceArcDemo, Log, TEXT("%s"), *Message);
	}
}
