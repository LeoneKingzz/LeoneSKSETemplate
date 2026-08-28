#include "hook.h"

namespace hooks
{
	bool DGD::GetBoolVariable(RE::Actor *a_actor, std::string a_string)
	{
		auto result = false;
		a_actor->GetGraphVariableBool(a_string, result);
		return result;
	}

	int DGD::GetIntVariable(RE::Actor *a_actor, std::string a_string)
	{
		auto result = 0;
		a_actor->GetGraphVariableInt(a_string, result);
		return result;
	}

	float DGD::GetFloatVariable(RE::Actor *a_actor, std::string a_string)
	{
		auto result = 0.0f;
		a_actor->GetGraphVariableFloat(a_string, result);
		return result;
	}

	bool IsInSpecialCombatState(RE::Actor *a_actor)
	{

		if (const auto base = a_actor->GetActorBase(); base)
		{
			for (const auto &factionInfo : base->factions)
			{
				if (factionInfo.faction && factionInfo.rank >= 0)
				{
					if (factionInfo.faction->HasSpecialCombatState())
					{
						return true;
					}
				}
			}
		}

		if (const auto factionChanges = a_actor->extraList.GetByType<RE::ExtraFactionChanges>(); factionChanges)
		{
			for (const auto &change : factionChanges->factionChanges)
			{
				if (change.faction && change.rank >= 0)
				{
					if (change.faction->HasSpecialCombatState())
					{
						return true;
					}
				}
			}
		}

		return false;
	};

	int GetPlayerFollowerCount()
	{
		int result = 0;

		if (!IsInSpecialCombatState(RE::PlayerCharacter::GetSingleton()))
		{
			if (const auto processLists = RE::ProcessLists::GetSingleton(); processLists)
			{
				for (auto &actorHandle : processLists->highActorHandles)
				{
					if (auto actor = actorHandle.get(); actor && actor->IsPlayerTeammate() && actor->Is3DLoaded() && !actor->HasKeywordString("ActorTypeHorse"))
					{
						result += 1;
					}
				}
			}
		}

		return result;
	}

	void AdjustDifficulty(Difficulty level)
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		player->GetGameStatsData().difficulty = (int)level;

		// logger::info("Diff {}", player->GetGameStatsData().difficulty);
	}

	void SetBaseDifficulty(int level){

		difficulty.BasePreference = (Difficulty)level;
	}

	class OurEventSink :
		public RE::BSTEventSink<RE::TESCombatEvent>
	{
		OurEventSink() = default;
		OurEventSink(const OurEventSink&) = delete;
		OurEventSink(OurEventSink&&) = delete;
		OurEventSink& operator=(const OurEventSink&) = delete;
		OurEventSink& operator=(OurEventSink&&) = delete;

	public:
		static OurEventSink* GetSingleton()
		{
			static OurEventSink singleton;
			return &singleton;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* event, RE::BSTEventSource<RE::TESCombatEvent>*){
			
			auto a_actor = event->actor->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			const auto followerCount = GetPlayerFollowerCount();

			switch (followerCount)
			{
			case 0:
				if (difficulty.currentlevel != difficulty.BasePreference)
				{
					difficulty.currentlevel = difficulty.BasePreference;

					AdjustDifficulty(difficulty.currentlevel);
				}
				break;

			default:

				if ((int)difficulty.currentlevel != (int)difficulty.BasePreference + (Settings::GetSingleton()->general.iDiffIncrement * followerCount))
				{
					if (((int)difficulty.BasePreference + (Settings::GetSingleton()->general.iDiffIncrement * followerCount)) > 5)
					{
						difficulty.currentlevel = Difficulty::Legendary;
					}
					else
					{
						difficulty.currentlevel = (Difficulty)((int)difficulty.BasePreference + (Settings::GetSingleton()->general.iDiffIncrement * followerCount));
					}

					AdjustDifficulty(difficulty.currentlevel);
				}
				break;
			}

			return RE::BSEventNotifyControl::kContinue;
		}
	};

	bool GetshouldHelp(const RE::Actor *p_ally, const RE::Actor *a_actor)
	{
		static RE::TESConditionItem cond;
		static std::once_flag flag;
		std::call_once(flag, [&]()
					   {
        cond.data.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetShouldHelp;
        cond.data.flags.opCode          = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
        cond.data.comparisonValue.f     = 1.0f; });

		ConditionParam cond_param;
		cond_param.form = const_cast<RE::TESObjectREFR *>(a_actor->As<RE::TESObjectREFR>());
		cond.data.functionData.params[0] = std::bit_cast<void *>(cond_param);

		RE::ConditionCheckParams params(const_cast<RE::TESObjectREFR *>(p_ally->As<RE::TESObjectREFR>()),
										const_cast<RE::TESObjectREFR *>(a_actor->As<RE::TESObjectREFR>()));
		return cond(params);
	}

	bool IsValidLifeState(RE::Actor *a_actor, bool checkDeath)
	{
		if (checkDeath)
		{
			switch (a_actor->AsActorState()->GetLifeState())
			{
			case RE::ACTOR_LIFE_STATE::kDying:
			case RE::ACTOR_LIFE_STATE::kDead:
				return false;

			default:
				return true;
			}
		}
		else
		{
			switch (a_actor->AsActorState()->GetLifeState())
			{
			case RE::ACTOR_LIFE_STATE::kBleedout:
			case RE::ACTOR_LIFE_STATE::kDying:
			case RE::ACTOR_LIFE_STATE::kDead:
			case RE::ACTOR_LIFE_STATE::kUnconcious:
			case RE::ACTOR_LIFE_STATE::kEssentialDown:
				return false;

			default:
				return true;
			}
		}
	}


	void DGD::install(){

		auto eventSink = OurEventSink::GetSingleton();

		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		
		eventSourceHolder->AddEventSink<RE::TESCombatEvent>(eventSink);
	}

	int DGD::GenerateRandomInt(int value_a, int value_b)
	{
		std::mt19937 generator(rd());
		std::uniform_int_distribution<int> dist(value_a, value_b);
		return dist(generator);
	}

	float DGD::GenerateRandomFloat(float value_a, float value_b)
	{
		std::mt19937 generator(rd());
		std::uniform_real_distribution<float> dist(value_a, value_b);
		return dist(generator);
	}
	double DGD::GenerateRandomDouble(double value_a, double value_b)
	{
		std::mt19937 generator(rd());
		std::uniform_real_distribution<double> dist(value_a, value_b);
		return dist(generator);
	}

	void Settings::Load(){
		constexpr auto path = "Data\\SKSE\\Plugins\\DynamicGameDifficulty.ini";

		CSimpleIniA ini;
		ini.SetUnicode();

		ini.LoadFile(path);

		general.Load(ini);

		ini.SaveFile(path);
	}

	

	void Settings::General_Settings::Load(CSimpleIniA &a_ini)
	{
		static const char *section = "General_Settings";

		auto DS = GetSingleton();

		DS->general.iBasePreference = static_cast<int>(a_ini.GetLongValue(section, "iBasePreference", DS->general.iBasePreference));
		a_ini.SetLongValue(section, "iBasePreference", DS->general.iBasePreference, ";Base difficulty preference with no followers. Range = 0-5. E.g. 0 = Novice | 5 = Legendary");

		DS->general.iDiffIncrement = static_cast<int>(a_ini.GetLongValue(section, "iDiffIncrement", DS->general.iDiffIncrement));
		a_ini.SetLongValue(section, "iDiffIncrement", DS->general.iDiffIncrement, ";The amount the increase the difficulty by per follower in player's party");

		DS->general.fDiffMultHPByPCVE = a_ini.GetDoubleValue(section, "fDiffMultHPByPCVE", DS->general.fDiffMultHPByPCVE);
		DS->general.fDiffMultHPByPCE = a_ini.GetDoubleValue(section, "fDiffMultHPByPCE", DS->general.fDiffMultHPByPCE);
		DS->general.fDiffMultHPByPCN = a_ini.GetDoubleValue(section, "fDiffMultHPByPCN", DS->general.fDiffMultHPByPCN);
		DS->general.fDiffMultHPByPCH = a_ini.GetDoubleValue(section, "fDiffMultHPByPCH", DS->general.fDiffMultHPByPCH);
		DS->general.fDiffMultHPByPCVH = a_ini.GetDoubleValue(section, "fDiffMultHPByPCVH", DS->general.fDiffMultHPByPCVH);
		DS->general.fDiffMultHPByPCL = a_ini.GetDoubleValue(section, "fDiffMultHPByPCL", DS->general.fDiffMultHPByPCL);

		a_ini.SetDoubleValue(section, "fDiffMultHPByPCVE", DS->general.fDiffMultHPByPCVE, ";Damage dealt on Novice");
		a_ini.SetDoubleValue(section, "fDiffMultHPByPCE", DS->general.fDiffMultHPByPCE, ";Damage dealt on Apprentrice");
		a_ini.SetDoubleValue(section, "fDiffMultHPByPCN", DS->general.fDiffMultHPByPCN, ";Damage dealt on Adept");
		a_ini.SetDoubleValue(section, "fDiffMultHPByPCH", DS->general.fDiffMultHPByPCH, ";Damage dealt on Expert");
		a_ini.SetDoubleValue(section, "fDiffMultHPByPCVH", DS->general.fDiffMultHPByPCVH, ";Damage dealt on Master");
		a_ini.SetDoubleValue(section, "fDiffMultHPByPCL", DS->general.fDiffMultHPByPCL, ";Damage dealt on Legendary");

		DS->general.fDiffMultHPToPCVE = a_ini.GetDoubleValue(section, "fDiffMultHPToPCVE", DS->general.fDiffMultHPToPCVE);
		DS->general.fDiffMultHPToPCE = a_ini.GetDoubleValue(section, "fDiffMultHPToPCE", DS->general.fDiffMultHPToPCE);
		DS->general.fDiffMultHPToPCN = a_ini.GetDoubleValue(section, "fDiffMultHPToPCN", DS->general.fDiffMultHPToPCN);
		DS->general.fDiffMultHPToPCH = a_ini.GetDoubleValue(section, "fDiffMultHPToPCH", DS->general.fDiffMultHPToPCH);
		DS->general.fDiffMultHPToPCVH = a_ini.GetDoubleValue(section, "fDiffMultHPToPCVH", DS->general.fDiffMultHPToPCVH);
		DS->general.fDiffMultHPToPCL = a_ini.GetDoubleValue(section, "fDiffMultHPToPCL", DS->general.fDiffMultHPToPCL);

		a_ini.SetDoubleValue(section, "fDiffMultHPToPCVE", DS->general.fDiffMultHPToPCVE, ";Damage received on Novice");
		a_ini.SetDoubleValue(section, "fDiffMultHPToPCE", DS->general.fDiffMultHPToPCE, ";Damage received on Apprentrice");
		a_ini.SetDoubleValue(section, "fDiffMultHPToPCN", DS->general.fDiffMultHPToPCN, ";Damage received on Adept");
		a_ini.SetDoubleValue(section, "fDiffMultHPToPCH", DS->general.fDiffMultHPToPCH, ";Damage received on Expert");
		a_ini.SetDoubleValue(section, "fDiffMultHPToPCVH", DS->general.fDiffMultHPToPCVH, ";Damage received on Master");
		a_ini.SetDoubleValue(section, "fDiffMultHPToPCL", DS->general.fDiffMultHPToPCL, ";Damage received on Legendary");

		SetBaseDifficulty(DS->general.iBasePreference);
	}

	void Settings::General_Settings::LoadSettings()
	{
		constexpr auto set_gmst = [](const char *a_name, float a_value)
		{
			if (auto gameSetting = RE::GameSettingCollection::GetSingleton()->GetSetting(a_name))
			{
				gameSetting->data.f = a_value;
			}
		};

		set_gmst("fDiffMultHPByPCVE", static_cast<float>(fDiffMultHPByPCVE));
		set_gmst("fDiffMultHPByPCE", static_cast<float>(fDiffMultHPByPCE));
		set_gmst("fDiffMultHPByPCN", static_cast<float>(fDiffMultHPByPCN));
		set_gmst("fDiffMultHPByPCH", static_cast<float>(fDiffMultHPByPCH));
		set_gmst("fDiffMultHPByPCVH", static_cast<float>(fDiffMultHPByPCVH));
		set_gmst("fDiffMultHPByPCL", static_cast<float>(fDiffMultHPByPCL));

		set_gmst("fDiffMultHPToPCVE", static_cast<float>(fDiffMultHPToPCVE));
		set_gmst("fDiffMultHPToPCE", static_cast<float>(fDiffMultHPToPCE));
		set_gmst("fDiffMultHPToPCN", static_cast<float>(fDiffMultHPToPCN));
		set_gmst("fDiffMultHPToPCH", static_cast<float>(fDiffMultHPToPCH));
		set_gmst("fDiffMultHPToPCVH", static_cast<float>(fDiffMultHPToPCVH));
		set_gmst("fDiffMultHPToPCL", static_cast<float>(fDiffMultHPToPCL));
	}
}
