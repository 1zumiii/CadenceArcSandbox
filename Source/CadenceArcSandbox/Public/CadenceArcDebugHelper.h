#pragma once
#include "CadenceArcSandbox/CadenceArcSandbox.h"
#include "Engine/Engine.h"

// Resolver 的每次调用和失败原因都记在编辑器的 Arc History 里，这里只处理它看不到的执行器侧问题：
// - Print：配置或初始化错误，必须让人马上看到，打在屏幕上并写日志；
// - Warn：History 里已经有记录的失败，只写日志备查，不刷屏。
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

	static void Warn(const FString& Message)
	{
		UE_LOG(LogCadenceArcDemo, Warning, TEXT("%s"), *Message);
	}
}
