// FCadenceArcHoldInputRouter 的确定性测试。时间全部由测试注入，不依赖 World、Timer 或输入设备。
//
// 被测契约（2026-09-22 与用户确认的接口）：
// - 每个带 Now 的调用都先 AdvanceInputTime(Now) 并把产生的请求交给 StartRequest，再处理本次输入。
// - Press：Tracker 产生新按下才记录 Tag -> {Token, Mode}；PressOnly 送 SubmitInput，HoldRelease 送
//   BeginInputHold。Begin 被拒绝时仍保留物理按住记录，直到真实松手或取消；重复按下被忽略。
// - Release：按记录的 Token 结束 Tracker 配对；HoldRelease 再调 ReleaseInputHold（NoMatchingHold 无副作用）。
// - Cancel / CancelAll：取消 Resolver 资格并结束 Tracker 配对，不生成 Released，不产生请求。
// - Resolver 无效（空指针或已被销毁）时所有调用都是安全的空操作，不调用 StartRequest。

#if WITH_DEV_AUTOMATION_TESTS

#include "Input/CadenceArcHoldInputRouter.h"

#include "Graph/CadenceArcGraph.h"
#include "Misc/AutomationTest.h"
#include "NativeGameplayTags.h"
#include "Resolver/CadenceArcResolver.h"

namespace CadenceArc::Sandbox::Tests
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Router_Action_Root, "CadenceArc.Sandbox.Test.Action.Root");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Router_Action_Light01, "CadenceArc.Sandbox.Test.Action.Light01");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Router_Action_Light02, "CadenceArc.Sandbox.Test.Action.Light02");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Router_Action_HeavyTap, "CadenceArc.Sandbox.Test.Action.HeavyTap");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Router_Action_HeavyCharged, "CadenceArc.Sandbox.Test.Action.HeavyCharged");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Router_Action_FinisherTap, "CadenceArc.Sandbox.Test.Action.FinisherTap");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Router_Action_FinisherCharged, "CadenceArc.Sandbox.Test.Action.FinisherCharged");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Router_Input_Light, "CadenceArc.Sandbox.Test.Input.Light");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Router_Input_Heavy, "CadenceArc.Sandbox.Test.Input.Heavy");

	// 全部用二进制精确值，避免浮点相加落在阈值边界上：
	// 按下 1.0 -> 蓄力起点 1.25、蓄满 1.5；保持 0 时 1.5 自动释放，保持 1.0 时 2.5 自动释放。
	constexpr double RouterChargeStartSeconds = 0.25;
	constexpr double RouterChargeFullSeconds = 0.5;

	static FCadenceArcTransition MakeRouterEdge(
		const FGameplayTag& InputTag, const FGameplayTag& Target, const ECadenceArcInputPhase Phase,
		const double MinSeconds = 0.0, const bool bHasMax = false, const double MaxSeconds = 0.0)
	{
		FCadenceArcTransition Edge;
		Edge.InputTag = InputTag;
		Edge.TargetActionTag = Target;
		Edge.InputPhase = Phase;
		if (Phase == ECadenceArcInputPhase::Released)
		{
			Edge.bUseDurationRange = true;
			Edge.DurationRange.MinHeldDurationSeconds = MinSeconds;
			Edge.DurationRange.bHasMaxHeldDuration = bHasMax;
			Edge.DurationRange.MaxHeldDurationSecondsExclusive = MaxSeconds;
		}
		return Edge;
	}

	// Light 为 Pressed 分支（PressOnly 路径），Heavy 为两档 Released 分支（HoldRelease 路径）。
	// Root 上没有 Released 的 Light 边，用来制造"Begin 被拒绝"。
	static void AddRouterNode(
		UCadenceArcGraph* Graph, const FGameplayTag& ActionTag, const FGameplayTag& LightTarget,
		const FGameplayTag& TapTarget, const FGameplayTag& ChargedTarget, const double MaxChargedHoldSeconds)
	{
		FCadenceArcNode& Node = Graph->Nodes.AddDefaulted_GetRef();
		Node.ActionTag = ActionTag;
		Node.Transitions.Add(MakeRouterEdge(Router_Input_Light, LightTarget, ECadenceArcInputPhase::Pressed));
		Node.Transitions.Add(MakeRouterEdge(Router_Input_Heavy, TapTarget, ECadenceArcInputPhase::Released,
		                                    0.0, true, RouterChargeFullSeconds));
		Node.Transitions.Add(MakeRouterEdge(Router_Input_Heavy, ChargedTarget, ECadenceArcInputPhase::Released,
		                                    RouterChargeFullSeconds));
		FCadenceArcHoldChargeConfig& Config = Node.HoldChargeConfigs.AddDefaulted_GetRef();
		Config.InputTag = Router_Input_Heavy;
		Config.ChargeStartSeconds = RouterChargeStartSeconds;
		Config.MaxChargedHoldSeconds = MaxChargedHoldSeconds;
	}

	static UCadenceArcResolver* MakeRouterResolver(FAutomationTestBase& Test, const double MaxChargedHoldSeconds = 1.0)
	{
		UCadenceArcGraph* Graph = NewObject<UCadenceArcGraph>();
		Graph->EntryActionTag = Router_Action_Root;
		Graph->Nodes.Reserve(8);
		AddRouterNode(Graph, Router_Action_Root, Router_Action_Light01,
		              Router_Action_HeavyTap, Router_Action_HeavyCharged, MaxChargedHoldSeconds);
		AddRouterNode(Graph, Router_Action_Light01, Router_Action_Light02,
		              Router_Action_FinisherTap, Router_Action_FinisherCharged, MaxChargedHoldSeconds);
		for (const FGameplayTag& Leaf : {
			     Router_Action_Light02.GetTag(), Router_Action_HeavyTap.GetTag(), Router_Action_HeavyCharged.GetTag(),
			     Router_Action_FinisherTap.GetTag(), Router_Action_FinisherCharged.GetTag()
		     })
		{
			Graph->Nodes.AddDefaulted_GetRef().ActionTag = Leaf;
		}

		UCadenceArcResolver* Resolver = NewObject<UCadenceArcResolver>();
		Test.TestEqual(TEXT("Router graph initializes"), static_cast<int32>(Resolver->Initialize(Graph)),
		               static_cast<int32>(ECadenceArcResolverInitResult::Success));
		return Resolver;
	}

	// 模拟宿主的 StartRequest：同步记录并接受请求，让 Resolver 立刻进入 Executing。
	struct FRouterHost
	{
		UCadenceArcResolver* Resolver = nullptr;
		TArray<FCadenceArcActionRequest> Started;

		void Start(const FCadenceArcActionRequest& Request)
		{
			Started.Add(Request);
			if (Resolver)
			{
				Resolver->NotifyActionStarted(Request.RequestId);
			}
		}
	};

	static bool ExpectStartedTargets(
		FAutomationTestBase& Test, const TCHAR* What, const FRouterHost& Host, const TArray<FGameplayTag>& Expected)
	{
		if (!Test.TestEqual(*FString::Printf(TEXT("%s start count"), What), Host.Started.Num(), Expected.Num()))
		{
			for (const FCadenceArcActionRequest& Request : Host.Started)
			{
				Test.AddInfo(FString::Printf(TEXT("%s started %s"), What, *Request.TargetActionTag.ToString()));
			}
			return false;
		}
		bool bPassed = true;
		for (int32 Index = 0; Index < Expected.Num(); ++Index)
		{
			bPassed &= Test.TestEqual(*FString::Printf(TEXT("%s start[%d] target"), What, Index),
			                          Host.Started[Index].TargetActionTag.ToString(), Expected[Index].ToString());
		}
		return bPassed;
	}

	static bool ExpectHoldState(
		FAutomationTestBase& Test, const TCHAR* What, const UCadenceArcResolver* Resolver, const bool bExpectHold)
	{
		const FString Label = FString::Printf(TEXT("%s resolver hold"), What);
		const bool bHasHold = Resolver->GetInputHoldSnapshot().bHasHold;
		return bExpectHold ? Test.TestTrue(*Label, bHasHold) : Test.TestFalse(*Label, bHasHold);
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FCadenceArcRouterPressOnlyTest,
		"CadenceArc.Sandbox.HoldRouter.PressOnlySubmitsOnPress",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FCadenceArcRouterPressOnlyTest::RunTest(const FString& Parameters)
	{
		UCadenceArcResolver* Resolver = MakeRouterResolver(*this);
		FRouterHost Host{Resolver};
		const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
		FCadenceArcHoldInputRouter Router(Resolver);

		Router.Press(Router_Input_Light, ECadenceArcInputMode::PressOnly, 1.0, Start);
		ExpectStartedTargets(*this, TEXT("PressOnly press"), Host, {Router_Action_Light01});
		TestTrue(TEXT("PressOnly press is tracked until release"), Router.IsTracking(Router_Input_Light));
		ExpectHoldState(*this, TEXT("PressOnly press"), Resolver, false);

		// 松手只结束物理配对，不会再次提交
		Router.Release(Router_Input_Light, 1.1, Start);
		ExpectStartedTargets(*this, TEXT("PressOnly release"), Host, {Router_Action_Light01});
		TestFalse(TEXT("PressOnly release ends tracking"), Router.IsTracking(Router_Input_Light));
		return !HasAnyErrors();
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FCadenceArcRouterHoldReleaseTest,
		"CadenceArc.Sandbox.HoldRouter.HoldReleaseResolvesOnRelease",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FCadenceArcRouterHoldReleaseTest::RunTest(const FString& Parameters)
	{
		// 短按：按下只取得资格，松手才按时长选普通档
		{
			UCadenceArcResolver* Resolver = MakeRouterResolver(*this);
			FRouterHost Host{Resolver};
			const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
			FCadenceArcHoldInputRouter Router(Resolver);

			Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 1.0, Start);
			ExpectStartedTargets(*this, TEXT("Hold press"), Host, {});
			ExpectHoldState(*this, TEXT("Hold press"), Resolver, true);
			TestTrue(TEXT("Hold press is tracked"), Router.IsTracking(Router_Input_Heavy));

			Router.Release(Router_Input_Heavy, 1.1, Start);
			ExpectStartedTargets(*this, TEXT("Tap release"), Host, {Router_Action_HeavyTap});
			ExpectHoldState(*this, TEXT("Tap release"), Resolver, false);
			TestFalse(TEXT("Tap release ends tracking"), Router.IsTracking(Router_Input_Heavy));
		}

		// 长按：蓄满后松手选长按档，duration 由 Tracker 计算而不是由宿主填写
		{
			UCadenceArcResolver* Resolver = MakeRouterResolver(*this);
			FRouterHost Host{Resolver};
			const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
			FCadenceArcHoldInputRouter Router(Resolver);

			Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 1.0, Start);
			Router.Advance(1.3, Start);
			TestEqual(TEXT("Advance reaches Charging"), static_cast<int32>(Resolver->GetInputHoldSnapshot().Stage),
			          static_cast<int32>(ECadenceArcHoldStage::Charging));
			Router.Release(Router_Input_Heavy, 2.0, Start);
			ExpectStartedTargets(*this, TEXT("Charged release"), Host, {Router_Action_HeavyCharged});
		}
		return !HasAnyErrors();
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FCadenceArcRouterAdvanceFirstTest,
		"CadenceArc.Sandbox.HoldRouter.AdvanceRunsBeforeInput",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FCadenceArcRouterAdvanceFirstTest::RunTest(const FString& Parameters)
	{
		// 保持 0：1.5 应自动释放。中间没有任何 Tick，直接在 2.0 按下另一个键。
		// Router 必须先推进：到期的蓄力攻击先开始，新按下才进入（此时窗口关闭，被忽略）。
		// 若不先推进，SubmitInput 会返回 InputTimeAdvanceRequired，两次输入都丢失。
		UCadenceArcResolver* Resolver = MakeRouterResolver(*this, 0.0);
		FRouterHost Host{Resolver};
		const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
		FCadenceArcHoldInputRouter Router(Resolver);

		Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 1.0, Start);
		Router.Press(Router_Input_Light, ECadenceArcInputMode::PressOnly, 2.0, Start);
		ExpectStartedTargets(*this, TEXT("Overdue release before new press"), Host, {Router_Action_HeavyCharged});
		TestEqual(TEXT("Overdue release is executing"), Resolver->GetCurrentActionTag().ToString(),
		          Router_Action_HeavyCharged.GetTag().ToString());

		// 物理松手晚到：资格已经兑现过，不会再攻击一次，但物理配对要正常结束
		Router.Release(Router_Input_Heavy, 2.1, Start);
		ExpectStartedTargets(*this, TEXT("Late physical release"), Host, {Router_Action_HeavyCharged});
		TestFalse(TEXT("Late physical release ends tracking"), Router.IsTracking(Router_Input_Heavy));
		TestTrue(TEXT("Other key is still tracked"), Router.IsTracking(Router_Input_Light));
		return !HasAnyErrors();
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FCadenceArcRouterAutoReleaseTest,
		"CadenceArc.Sandbox.HoldRouter.TickAutoReleaseOnce",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FCadenceArcRouterAutoReleaseTest::RunTest(const FString& Parameters)
	{
		UCadenceArcResolver* Resolver = MakeRouterResolver(*this, 0.0);
		FRouterHost Host{Resolver};
		const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
		FCadenceArcHoldInputRouter Router(Resolver);

		Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 1.0, Start);
		Router.Advance(1.4, Start);
		ExpectStartedTargets(*this, TEXT("Before the deadline"), Host, {});

		// Tick 推进到截止之后：自动释放一次，请求交给宿主
		Router.Advance(1.6, Start);
		ExpectStartedTargets(*this, TEXT("Tick auto release"), Host, {Router_Action_HeavyCharged});

		// 重复推进与随后的物理松手都不会产生第二次攻击
		Router.Advance(1.7, Start);
		Router.Release(Router_Input_Heavy, 2.0, Start);
		ExpectStartedTargets(*this, TEXT("After auto release"), Host, {Router_Action_HeavyCharged});
		TestFalse(TEXT("Physical release still ends tracking"), Router.IsTracking(Router_Input_Heavy));
		return !HasAnyErrors();
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FCadenceArcRouterMultiTagTest,
		"CadenceArc.Sandbox.HoldRouter.TagsRouteIndependently",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FCadenceArcRouterMultiTagTest::RunTest(const FString& Parameters)
	{
		UCadenceArcResolver* Resolver = MakeRouterResolver(*this);
		FRouterHost Host{Resolver};
		const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
		FCadenceArcHoldInputRouter Router(Resolver);

		// Heavy 仍处于 Holding（1.1 < 1.25），Light 的按下被接受并替换掉 Heavy 的资格
		Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 1.0, Start);
		Router.Press(Router_Input_Light, ECadenceArcInputMode::PressOnly, 1.1, Start);
		ExpectStartedTargets(*this, TEXT("Light replaces holding heavy"), Host, {Router_Action_Light01});
		TestTrue(TEXT("Both keys are physically tracked"),
		         Router.IsTracking(Router_Input_Heavy) && Router.IsTracking(Router_Input_Light));

		// Heavy 松手必须用 Heavy 自己的 Token：只结束 Heavy 的配对，被替换的资格不产生攻击
		Router.Release(Router_Input_Heavy, 1.3, Start);
		ExpectStartedTargets(*this, TEXT("Replaced heavy release"), Host, {Router_Action_Light01});
		TestFalse(TEXT("Heavy release ends heavy tracking"), Router.IsTracking(Router_Input_Heavy));
		TestTrue(TEXT("Heavy release leaves light tracked"), Router.IsTracking(Router_Input_Light));

		Router.Release(Router_Input_Light, 1.4, Start);
		TestFalse(TEXT("Light release ends light tracking"), Router.IsTracking(Router_Input_Light));
		ExpectStartedTargets(*this, TEXT("Final"), Host, {Router_Action_Light01});
		return !HasAnyErrors();
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FCadenceArcRouterRejectedBeginTest,
		"CadenceArc.Sandbox.HoldRouter.RejectedBeginKeepsPhysicalPress",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FCadenceArcRouterRejectedBeginTest::RunTest(const FString& Parameters)
	{
		UCadenceArcResolver* Resolver = MakeRouterResolver(*this);
		FRouterHost Host{Resolver};
		const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
		FCadenceArcHoldInputRouter Router(Resolver);

		// Root 上 Light 没有 Released 边：Begin 被拒绝，但物理按住仍要跟踪到真实松手
		Router.Press(Router_Input_Light, ECadenceArcInputMode::HoldRelease, 1.0, Start);
		ExpectHoldState(*this, TEXT("Rejected begin"), Resolver, false);
		TestTrue(TEXT("Rejected begin keeps the physical press"), Router.IsTracking(Router_Input_Light));
		Router.Release(Router_Input_Light, 1.2, Start);
		TestFalse(TEXT("Release after rejected begin ends tracking"), Router.IsTracking(Router_Input_Light));
		ExpectStartedTargets(*this, TEXT("Rejected begin"), Host, {});

		// 重复按下被 Tracker 忽略，不会替换已授予的资格
		Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 2.0, Start);
		const FCadenceArcHoldSnapshot First = Resolver->GetInputHoldSnapshot();
		Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 2.1, Start);
		const FCadenceArcHoldSnapshot Second = Resolver->GetInputHoldSnapshot();
		TestTrue(TEXT("Repeated press keeps the token"), First.Token == Second.Token);
		TestEqual(TEXT("Repeated press keeps the press time"), Second.PressedTimestampSeconds, 2.0);
		return !HasAnyErrors();
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FCadenceArcRouterCancelTest,
		"CadenceArc.Sandbox.HoldRouter.CancelNeverReleases",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FCadenceArcRouterCancelTest::RunTest(const FString& Parameters)
	{
		// 单键取消：蓄力中取消，不合成松手；之后的松手与推进都不会攻击
		{
			UCadenceArcResolver* Resolver = MakeRouterResolver(*this);
			FRouterHost Host{Resolver};
			const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
			FCadenceArcHoldInputRouter Router(Resolver);

			Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 1.0, Start);
			Router.Advance(1.3, Start);
			Router.Cancel(Router_Input_Heavy);
			ExpectHoldState(*this, TEXT("Cancel"), Resolver, false);
			TestFalse(TEXT("Cancel ends tracking"), Router.IsTracking(Router_Input_Heavy));

			Router.Release(Router_Input_Heavy, 2.0, Start);
			Router.Advance(10.0, Start);
			ExpectStartedTargets(*this, TEXT("After cancel"), Host, {});

			// 取消之后同一个键可以重新按下
			Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 11.0, Start);
			ExpectHoldState(*this, TEXT("Press after cancel"), Resolver, true);
		}

		// 统一清理：所有按住的键和资格都清掉，不生成 Released；之后仍可正常使用
		{
			UCadenceArcResolver* Resolver = MakeRouterResolver(*this);
			FRouterHost Host{Resolver};
			const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
			FCadenceArcHoldInputRouter Router(Resolver);

			Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 1.0, Start);
			Router.Press(Router_Input_Light, ECadenceArcInputMode::HoldRelease, 1.05, Start); // Begin 被拒绝，仍跟踪
			TestTrue(TEXT("Both keys tracked before CancelAll"),
			         Router.IsTracking(Router_Input_Heavy) && Router.IsTracking(Router_Input_Light));

			Router.CancelAll();
			ExpectHoldState(*this, TEXT("CancelAll"), Resolver, false);
			TestFalse(TEXT("CancelAll clears heavy"), Router.IsTracking(Router_Input_Heavy));
			TestFalse(TEXT("CancelAll clears light"), Router.IsTracking(Router_Input_Light));

			Router.Release(Router_Input_Heavy, 1.2, Start);
			Router.Release(Router_Input_Light, 1.2, Start);
			Router.Advance(10.0, Start);
			ExpectStartedTargets(*this, TEXT("After CancelAll"), Host, {});

			Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 11.0, Start);
			ExpectHoldState(*this, TEXT("Press after CancelAll"), Resolver, true);
			TestTrue(TEXT("Press after CancelAll is tracked"), Router.IsTracking(Router_Input_Heavy));
		}
		return !HasAnyErrors();
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FCadenceArcRouterAcrossCompletionTest,
		"CadenceArc.Sandbox.HoldRouter.HoldSurvivesCompletion",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FCadenceArcRouterAcrossCompletionTest::RunTest(const FString& Parameters)
	{
		// 宿主完整时序：动作执行中开窗按下 -> 关窗 -> 先推进再 Completed -> 资格保留 -> 松手出长按攻击
		UCadenceArcResolver* Resolver = MakeRouterResolver(*this);
		FRouterHost Host{Resolver};
		const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
		FCadenceArcHoldInputRouter Router(Resolver);

		Router.Press(Router_Input_Light, ECadenceArcInputMode::PressOnly, 0.4, Start);
		Router.Release(Router_Input_Light, 0.5, Start);
		if (!ExpectStartedTargets(*this, TEXT("Light01 starts"), Host, {Router_Action_Light01})) { return false; }
		const int64 RequestId = Host.Started[0].RequestId;

		Resolver->OpenBufferWindow(RequestId);
		Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 1.0, Start);
		ExpectHoldState(*this, TEXT("Grant inside the window"), Resolver, true);
		Resolver->CloseBufferWindow(RequestId);

		// HandleActionCompleted 的约定：同一个 Now 先推进，再通知完成
		const double CompletionTime = 1.4;
		Router.Advance(CompletionTime, Start);
		const FCadenceArcActionCompletionOutcome Completed = Resolver->NotifyActionCompleted(RequestId, CompletionTime);
		TestEqual(TEXT("Completion handshake"), static_cast<int32>(Completed.GetHandshakeResult()),
		          static_cast<int32>(ECadenceArcHandshakeResult::Success));
		TestEqual(TEXT("Completion keeps the hold"), static_cast<int32>(Completed.GetBufferConsumptionReason()),
		          static_cast<int32>(ECadenceArcResolutionReason::WaitingForRelease));
		ExpectHoldState(*this, TEXT("After completion"), Resolver, true);

		Router.Release(Router_Input_Heavy, 2.0, Start);
		ExpectStartedTargets(*this, TEXT("Release after completion"), Host,
		                     {Router_Action_Light01, Router_Action_FinisherCharged});
		return !HasAnyErrors();
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FCadenceArcRouterInvalidResolverTest,
		"CadenceArc.Sandbox.HoldRouter.InvalidResolverIsNoOp",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	bool FCadenceArcRouterInvalidResolverTest::RunTest(const FString& Parameters)
	{
		// 空 Resolver：所有调用安全返回，不跟踪、不启动
		{
			FRouterHost Host;
			const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
			FCadenceArcHoldInputRouter Router(nullptr);
			Router.Advance(1.0, Start);
			Router.Press(Router_Input_Light, ECadenceArcInputMode::PressOnly, 1.0, Start);
			Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 1.0, Start);
			TestFalse(TEXT("Null resolver does not track"), Router.IsTracking(Router_Input_Light));
			Router.Release(Router_Input_Light, 1.1, Start);
			Router.Cancel(Router_Input_Heavy);
			Router.CancelAll();
			ExpectStartedTargets(*this, TEXT("Null resolver"), Host, {});
		}

		// Resolver 在按住期间被销毁（例如宿主重建了它）：晚到的回调不能崩溃，也不能启动动作
		{
			UCadenceArcResolver* Resolver = MakeRouterResolver(*this, 0.0);
			FRouterHost Host{Resolver};
			FCadenceArcHoldInputRouter Router(Resolver);
			const auto StartBeforeDestroy = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
			Router.Press(Router_Input_Heavy, ECadenceArcInputMode::HoldRelease, 1.0, StartBeforeDestroy);
			ExpectHoldState(*this, TEXT("Hold before destroy"), Resolver, true);

			Resolver->MarkAsGarbage();
			Host.Resolver = nullptr; // 宿主也不应再碰已销毁的对象
			const auto Start = [&Host](const FCadenceArcActionRequest& Request) { Host.Start(Request); };
			Router.Advance(2.0, Start); // 若仍访问旧 Resolver，这里本会自动释放并启动动作
			Router.Release(Router_Input_Heavy, 2.1, Start);
			Router.CancelAll();
			ExpectStartedTargets(*this, TEXT("Destroyed resolver"), Host, {});
		}
		return !HasAnyErrors();
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
