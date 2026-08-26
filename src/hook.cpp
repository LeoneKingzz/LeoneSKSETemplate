#include "hook.h"

namespace hooks
{
	void OnMeleeHitHook::Set_iFrames(RE::Actor* actor)
	{
		actor->SetGraphVariableBool("bIframeActive", true);
		actor->SetGraphVariableBool("bInIframe", true);
	}

	void OnMeleeHitHook::Reset_iFrames(RE::Actor* actor)
	{
		actor->SetGraphVariableBool("bIframeActive", false);
		actor->SetGraphVariableBool("bInIframe", false);
	}

	void OnMeleeHitHook::dispelEffect(RE::MagicItem *spellForm, RE::Actor *a_actor)
	{
		if (const auto targetActor = a_actor->AsMagicTarget(); targetActor)
		{
			if (spellForm && spellForm->effects[0] && spellForm->effects[0]->baseEffect && targetActor->HasMagicEffect(spellForm->effects[0]->baseEffect))
			{
				if (auto activeEffects = targetActor->GetActiveEffectList(); activeEffects)
				{
					for (const auto &effect : *activeEffects)
					{
						if (effect && effect->spell && effect->spell == spellForm)
						{
							effect->Dispel(true);
						}
					}
				}
			}
		}
	}

	bool Has_Magiceffect_Keyword(const RE::Actor *a_actor, const RE::BGSKeyword *a_key)
	{
		static RE::TESConditionItem cond;
		static std::once_flag flag;
		std::call_once(flag, [&]()
					   {
        cond.data.functionData.function = RE::FUNCTION_DATA::FunctionID::kHasMagicEffectKeyword;
        cond.data.flags.opCode          = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
		cond.data.object                = RE::CONDITIONITEMOBJECT::kSelf;
        cond.data.comparisonValue.f     = 0.0f; });

		ConditionParam cond_param;
		cond_param.form = const_cast<RE::BGSKeyword *>(a_key->As<RE::BGSKeyword>());
		cond.data.functionData.params[0] = std::bit_cast<void *>(cond_param);

		RE::ConditionCheckParams params(const_cast<RE::TESObjectREFR *>(a_actor->As<RE::TESObjectREFR>()), nullptr);
		return cond(params);
	}

	bool HasBoundWeaponEquipped(const RE::Actor *a_actor, RE::MagicSystem::CastingSource type)
	{
		static RE::TESConditionItem cond;
		static std::once_flag flag;
		std::call_once(flag, [&]()
					   {
        cond.data.functionData.function = RE::FUNCTION_DATA::FunctionID::kHasBoundWeaponEquipped;
        cond.data.flags.opCode          = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
        cond.data.object                = RE::CONDITIONITEMOBJECT::kSelf;
        cond.data.comparisonValue.f     = 0.0f; });

		ConditionParam cond_param;
		cond_param.i = static_cast<int32_t>(type);
		cond.data.functionData.params[0] = std::bit_cast<void *>(cond_param);

		RE::ConditionCheckParams params(const_cast<RE::TESObjectREFR *>(a_actor->As<RE::TESObjectREFR>()), nullptr);
		return cond(params);
	}

	bool GetLineOfSight(const RE::Actor *a_actor, const RE::Actor *a_target, float a_comparison_value)
	{
		static RE::TESConditionItem cond;
		static std::once_flag flag;
		std::call_once(flag, [&]()
					   {
        cond.data.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetLineOfSight;
        cond.data.flags.opCode          = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
        cond.data.comparisonValue.f     = a_comparison_value; });

		ConditionParam cond_param;
		cond_param.form = const_cast<RE::TESObjectREFR *>(a_target->As<RE::TESObjectREFR>());
		cond.data.functionData.params[0] = std::bit_cast<void *>(cond_param);

		RE::ConditionCheckParams params(const_cast<RE::TESObjectREFR *>(a_actor->As<RE::TESObjectREFR>()),
										const_cast<RE::TESObjectREFR *>(a_target->As<RE::TESObjectREFR>()));
		return cond(params);
	}

	bool GetIsGhost(const RE::Actor *a_actor, float a_comparison_value)
	{
		static RE::TESConditionItem cond;
		static std::once_flag flag;
		std::call_once(flag, [&]()
					   {
        cond.data.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetIsGhost;
        cond.data.flags.opCode          = RE::CONDITION_ITEM_DATA::OpCode::kEqualTo;
		cond.data.object                = RE::CONDITIONITEMOBJECT::kSelf;
        cond.data.comparisonValue.f     = a_comparison_value; });

		RE::ConditionCheckParams params(const_cast<RE::TESObjectREFR *>(a_actor->As<RE::TESObjectREFR>()), nullptr);
		return cond(params);
	}

	float OnMeleeHitHook::GetActorValuePercent(RE::Actor *a_actor, RE::ActorValue a_value)
	{
		auto result = 0.0f;
		auto cur_value = a_actor->AsActorValueOwner()->GetActorValue(a_value);
		auto perm_value = a_actor->AsActorValueOwner()->GetPermanentActorValue(a_value);

		if (perm_value != 0.0f)
		{
			result = cur_value / perm_value;
		}
		return result;
	}

	bool OnMeleeHitHook::isHumanoid(RE::Actor* a_actor)
	{
		auto bodyPartData = a_actor->GetRace() ? a_actor->GetRace()->bodyPartData : nullptr;
		return bodyPartData && bodyPartData->GetFormID() == 0x1d;
	}

	bool OnMeleeHitHook::isPowerAttacking(RE::Actor* a_actor)
	{
		auto currentProcess = a_actor->GetActorRuntimeData().currentProcess;
		if (currentProcess) {
			auto highProcess = currentProcess->high;
			if (highProcess) {
				auto attackData = highProcess->attackData;
				if (attackData) {
					auto flags = attackData->data.flags;
					return flags.any(RE::AttackData::AttackFlag::kPowerAttack);
				}
			}
		}
		return false;
	}

	void OnMeleeHitHook::UpdateCombatTarget(RE::Actor* a_actor){
		auto CTarget = a_actor->GetActorRuntimeData().currentCombatTarget.get().get();
		if (!CTarget) {
			auto combatGroup = a_actor->GetCombatGroup();
			if (combatGroup) {
				for (auto it = combatGroup->targets.begin(); it != combatGroup->targets.end(); ++it) {
					if (it->targetHandle && it->targetHandle.get().get()) {
						a_actor->GetActorRuntimeData().currentCombatTarget = it->targetHandle.get().get();
						break;
					}
					continue;
				}
			}
		}
		//a_actor->UpdateCombat();
	}


	bool OnMeleeHitHook::IsCasting(RE::Actor* a_actor)
	{
		bool result = false;

		if ((GetBoolVariable(a_actor, "IsCastingRight")) || (GetBoolVariable(a_actor, "IsCastingLeft")) || (GetBoolVariable(a_actor, "IsCastingDual")))
		{
			result = true;
		}

		return result;
	}

	void OnMeleeHitHook::InterruptAttack(RE::Actor* a_actor){
		a_actor->NotifyAnimationGraph("attackStop");
		a_actor->NotifyAnimationGraph("recoilStop");
		a_actor->NotifyAnimationGraph("bashStop");
		a_actor->NotifyAnimationGraph("blockStop");
		a_actor->NotifyAnimationGraph("staggerStop");
	}

	std::vector<RE::TESForm*> OnMeleeHitHook::GetEquippedForm(RE::Actor* actor)
	{
		std::vector<RE::TESForm*> Hen;

		auto limboform = actor->GetActorRuntimeData().currentProcess;

		if (limboform && limboform->GetEquippedLeftHand()) {
			Hen.push_back(limboform->GetEquippedLeftHand());
		}
		if (limboform && limboform->GetEquippedRightHand()) {
			Hen.push_back(limboform->GetEquippedRightHand());
		}

		return Hen;
	}

	bool OnMeleeHitHook::IsWeaponOut(RE::Actor *actor)
	{
		bool result = false;
		auto form_list = GetEquippedForm(actor);

		if (!form_list.empty()) {
			for (auto form : form_list) {
				if (form) {
					switch (*form->formType) {
					case RE::FormType::Weapon:
						if (const auto equippedWeapon = form->As<RE::TESObjectWEAP>()) {
							switch (equippedWeapon->GetWeaponType()) {
							case RE::WEAPON_TYPE::kOneHandSword:
							case RE::WEAPON_TYPE::kOneHandDagger:
							case RE::WEAPON_TYPE::kOneHandAxe:
							case RE::WEAPON_TYPE::kOneHandMace:
							case RE::WEAPON_TYPE::kTwoHandSword:
							case RE::WEAPON_TYPE::kTwoHandAxe:
							case RE::WEAPON_TYPE::kBow:
							case RE::WEAPON_TYPE::kCrossbow:
								result = true;
								break;
							default:
								break;
							}
						}
						break;
					case RE::FormType::Armor:
						if (auto equippedShield = form->As<RE::TESObjectARMO>()) {
							result = true;
						}
						break;
					default:
						break;
					}
					if (result) {
						break;
					}
				}
				continue;
			}
		}
		return result;
	}

	bool OnMeleeHitHook::IsMeleeOnly(RE::Actor* a_actor)
	{
		using TYPE = RE::CombatInventoryItem::TYPE;

		auto result = false;

		auto combatCtrl = a_actor->GetActorRuntimeData().combatController;
		auto CombatInv = combatCtrl ? combatCtrl->inventory : nullptr;
		if (CombatInv) {
			for (const auto item : CombatInv->equippedItems) {
				if (item.item) {
					switch (item.item->GetType()) {
					case TYPE::kMelee:
						result = true;
						break;
					default:
						break;
					}
				}
				if (result){
					break;
				}
			}
		}

		return result;
	}

	bool OnMeleeHitHook::IsEquipped(RE::Actor *a_actor, RE::TESBoundObject * a_object)
	{
		bool result = false;

		if (auto combatCtrl = a_actor->GetActorRuntimeData().combatController; combatCtrl)
		{
			if (auto CombatInv = combatCtrl->inventory; CombatInv)
			{
				for (auto &invent_item : CombatInv->equippedItems)
				{
					if (invent_item.item && invent_item.item.get() && invent_item.item.get()->item && invent_item.item.get()->item == a_object)
					{
						result = true;

						break;
					}
				}
			}
		}
		return result;
	}

	void OnMeleeHitHook::EquipfromInvent(RE::Actor* a_actor, RE::FormID a_formID)
	{
		auto inv = a_actor->GetInventory();
		for (auto& [item, data] : inv) {
			const auto& [count, entry] = data;
			if (count > 0 && entry->object->formID == a_formID) {
				RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, entry->object);
				break;
			}
			continue;
		}
	}

	float OnMeleeHitHook::AV_Mod(RE::Actor* a_actor, int actor_value, float input, float mod)
	{
		if (actor_value > 0){
			int k;
			for (k = 0; k <= actor_value; k += 1) {
				input += mod;
			}
		}

		return input;
	}

	bool OnMeleeHitHook::GetBoolVariable(RE::Actor *a_actor, std::string a_string)
	{
		auto result = false;
		a_actor->GetGraphVariableBool(a_string, result);
		return result;
	}

	int OnMeleeHitHook::GetIntVariable(RE::Actor *a_actor, std::string a_string)
	{
		auto result = 0;
		a_actor->GetGraphVariableInt(a_string, result);
		return result;
	}

	float OnMeleeHitHook::GetFloatVariable(RE::Actor *a_actor, std::string a_string)
	{
		auto result = 0.0f;
		a_actor->GetGraphVariableFloat(a_string, result);
		return result;
	}

	int GetPlayerFollowerCount()
	{
		int result = 0;

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

		return result;
	}

	void AdjustDifficulty(Difficulty level)
	{
		constexpr auto set_gmst = [](const char *a_name, float a_value)
		{
			if (auto gameSetting = RE::GameSettingCollection::GetSingleton()->GetSetting(a_name))
			{
				gameSetting->data.f = a_value;
			}
		};

		float damageBy = 1.0f;
		float damageTo = 1.0f;

		switch (level)
		{
		case Difficulty::Adept:
			damageBy = 1.0f;
			damageTo = 1.0f;
			break;

		case Difficulty::Expert:
			damageBy = 0.75f;
			damageTo = 1.17f;
			break;

		case Difficulty::Master:
			damageBy = 0.50f;
			damageTo = 1.34f;
			break;

		case Difficulty::Legendary:
			damageBy = 0.25f;
			damageTo = 1.51f;
			break;

		default:
			break;
		}
		set_gmst("fDiffMultHPByPCVE", damageBy);
		set_gmst("fDiffMultHPByPCE", damageBy);
		set_gmst("fDiffMultHPByPCN", damageBy);
		set_gmst("fDiffMultHPByPCH", damageBy);
		set_gmst("fDiffMultHPByPCVH", damageBy);
		set_gmst("fDiffMultHPByPCL", damageBy);

		set_gmst("fDiffMultHPToPCVE", damageTo);
		set_gmst("fDiffMultHPToPCE", damageTo);
		set_gmst("fDiffMultHPToPCN", damageTo);
		set_gmst("fDiffMultHPToPCH", damageTo);
		set_gmst("fDiffMultHPToPCVH", damageTo);
		set_gmst("fDiffMultHPToPCL", damageTo);
	}

	class OurEventSink :
		public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>,
		public RE::BSTEventSink<RE::TESEquipEvent>,
		public RE::BSTEventSink<RE::TESCombatEvent>,
		public RE::BSTEventSink<RE::TESActorLocationChangeEvent>,
		public RE::BSTEventSink<RE::TESSpellCastEvent>,
		public RE::BSTEventSink<RE::TESDeathEvent>,
		public RE::BSTEventSink<SKSE::ModCallbackEvent>
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

		RE::BSEventNotifyControl ProcessEvent(const RE::TESSwitchRaceCompleteEvent* event, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*)
		{
			auto a_actor = event->subject->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent *event, RE::BSTEventSource<RE::TESDeathEvent> *)
		{
			auto a_actor = event->actorDying->As<RE::Actor>();

			if (!a_actor)
			{
				return RE::BSEventNotifyControl::kContinue;
			}

			if (a_actor->IsPlayerRef()){
				
			}else{
				
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*){

			if (!event || !event->actor)
			{
				return RE::BSEventNotifyControl::kContinue;
			}
			auto a_actor = event->actor->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!(OnMeleeHitHook::GetBoolVariable(a_actor, "od") && OnMeleeHitHook::GetBoolVariable(a_actor, "dum")))
			{
				return RE::BSEventNotifyControl::kContinue;
			}

			auto item = RE::TESForm::LookupByID<RE::TESBoundObject>(event->baseObject);
			if (!item)
			{
				return RE::BSEventNotifyControl::kContinue;
			}
			if (event->equipped)
			{
				if(item->IsMagicItem() && item->Is(RE::FormType::Spell)){

					// if (OnMeleeHitHook::GetBoolVariable(a_actor, "dum"))
					// {
					// 	a_actor->SetGraphVariableBool("dum", false);
					// 	OnMeleeHitHook::GetSingleton()->RegisterforUpdate(a_actor, std::make_tuple(nullptr, std::chrono::steady_clock::now(), 2000ms, "Dum_Update"));
					// }
				}
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* event, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
		{
			if (!event || event->eventName.empty())
			{
				return RE::BSEventNotifyControl::kContinue;
			}

			if (event->eventName == "CustomYEvent")
			{
				logger::info("Custom Event Recieved");

				// Settings::GetSingleton()->Load();
			}


			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* event, RE::BSTEventSource<RE::TESCombatEvent>*){
			
			auto a_actor = event->actor->As<RE::Actor>();

			if (!a_actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			// switch (event->newState.get()) {
			// case RE::ACTOR_COMBAT_STATE::kCombat:

			// 	break;

			// case RE::ACTOR_COMBAT_STATE::kSearching:

			// 	break;

			// case RE::ACTOR_COMBAT_STATE::kNone:


			// 	break;

			// default:
			// 	break;
			// }

			const auto followerCount = GetPlayerFollowerCount();

			switch (followerCount)
			{
			case 0:
				if (difficulty.level != Difficulty::Adept)
				{
					difficulty.level = Difficulty::Adept;

					AdjustDifficulty(Difficulty::Adept);
				}
				break;

			case 1:
				if (difficulty.level != Difficulty::Expert)
				{
					difficulty.level = Difficulty::Expert;

					AdjustDifficulty(Difficulty::Expert);
				}
				break;

			case 2:
				if (difficulty.level != Difficulty::Master)
				{
					difficulty.level = Difficulty::Master;

					AdjustDifficulty(Difficulty::Master);
				}
				break;

			default:

				if (followerCount >= 3)
				{
					if (difficulty.level != Difficulty::Legendary)
					{
						difficulty.level = Difficulty::Legendary;

						AdjustDifficulty(Difficulty::Legendary);
					}
				}
				break;
			}

			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESActorLocationChangeEvent* event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*)
		{
			auto a_actor = event->actor->As<RE::Actor>();

			if (!a_actor || !a_actor->IsPlayerRef()) {
				return RE::BSEventNotifyControl::kContinue;
			}


			return RE::BSEventNotifyControl::kContinue;
		}

		RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* event, RE::BSTEventSource<RE::TESSpellCastEvent>*)
		{
			if (!event || !event->object)
			{
				return RE::BSEventNotifyControl::kContinue;
			}
			auto a_actor = event->object->As<RE::Actor>();

			if (!a_actor)
			{
				return RE::BSEventNotifyControl::kContinue;
			}

			if (!OnMeleeHitHook::GetBoolVariable(a_actor, "dya"))
			{
				return RE::BSEventNotifyControl::kContinue;
			}

			auto eSpell = RE::TESForm::LookupByID(event->spell);

			if (eSpell && eSpell->Is(RE::FormType::Spell)) {
				auto rSpell = eSpell->As<RE::SpellItem>();
				switch (rSpell->GetSpellType()) {
				case RE::MagicSystem::SpellType::kSpell:

					break;

				default:
					break;
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	float OnMeleeHitHook::get_angle_he_me(RE::Actor *me, RE::Actor *he, RE::BGSAttackData *attackdata){
		auto he_me = PolarAngle(me->GetPosition() - he->GetPosition());
		auto head = PolarAngle(he->GetHeading(false) * 180.0f / PI);
		if (attackdata)
			head = head.add(attackdata->data.attackAngle);
		auto angle = he_me.sub(head).to_normangle();
		return angle;
	}

	RE::BGSAttackData* OnMeleeHitHook::get_attackData(RE::Actor *a){
		if (!a->GetActorRuntimeData().currentProcess || !a->GetActorRuntimeData().currentProcess->high)
			return nullptr;
		return a->GetActorRuntimeData().currentProcess->high->attackData.get();
	}

	void OnMeleeHitHook::getBodyPos(RE::Actor *a_actor, RE::NiPoint3 &pos)
	{
		if (!a_actor->GetActorRuntimeData().race) {
			return;
		}
		RE::BGSBodyPart* bodyPart = a_actor->GetActorRuntimeData().race->bodyPartData->parts[0];
		if (!bodyPart) {
			return;
		}
		auto targetPoint = a_actor->GetNodeByName(bodyPart->targetName.c_str());
		if (!targetPoint) {
			return;
		}

		pos = targetPoint->world.translate;
	}


	float OnMeleeHitHook::get_personal_survivalRatio(RE::Actor *protagonist, RE::Actor *combat_target)
	{
		float result = 0.0f;
		float MyTeam_total_threat = 0.0f;
		float EnemyTeam_total_threat = 0.0f;
		float personal_threat = 0.0f;

		if (const auto combatGroup = protagonist->GetCombatGroup())
		{
			for (auto &memberData : combatGroup->members)
			{
				if (auto ally = memberData.memberHandle.get(); ally)
				{
					if (auto value = memberData.threatValue; value)
					{
						MyTeam_total_threat += value;
						if (ally.get() == protagonist)
						{
							personal_threat += value;
						}
					}
				}
				continue;
			}
		}

		if (const auto combatGroup = combat_target->GetCombatGroup())
		{
			for (auto &memberData : combatGroup->members)
			{
				if (auto ally = memberData.memberHandle.get(); ally)
				{
					if (auto value = memberData.threatValue; value)
					{
						EnemyTeam_total_threat += value;
					}
				}
				continue;
			}
		}

		if (MyTeam_total_threat > 0 && EnemyTeam_total_threat > 0 && personal_threat > 0)
		{

			auto personal_survival = personal_threat / EnemyTeam_total_threat;
			auto Enemy_groupSurvival = EnemyTeam_total_threat / MyTeam_total_threat;

			result = personal_survival / Enemy_groupSurvival;
		}

		return result;
	}
	float OnMeleeHitHook::get_personal_threatRatio(RE::Actor *protagonist, RE::Actor *combat_target)
	{
		float result = 0.0f;
		float personal_threat = 0.0f;
		float CTarget_threat = 0.0f;

		if (const auto combatGroup = protagonist->GetCombatGroup())
		{
			for (auto &memberData : combatGroup->members)
			{
				if (auto ally = memberData.memberHandle.get(); ally)
				{
					if (ally.get() == protagonist)
					{
						if (auto value = memberData.threatValue; value)
						{
							personal_threat += value;
							break;
						}
					}
				}
				continue;
			}
		}

		if (const auto combatGroup = combat_target->GetCombatGroup())
		{
			for (auto &memberData : combatGroup->members)
			{
				if (auto ally = memberData.memberHandle.get(); ally)
				{
					if (ally.get() == combat_target)
					{
						if (auto value = memberData.threatValue; value)
						{
							CTarget_threat += value;
							break;
						}
					}
				}
				continue;
			}
		}

		if (personal_threat > 0 && CTarget_threat > 0)
		{
			result = personal_threat / CTarget_threat;
		}

		return result;
	}
	float OnMeleeHitHook::confidence_threshold(RE::Actor *a_actor, int confidence, bool inverse)
	{
		float result = 0.0f;

		if (inverse)
		{
			switch (confidence)
			{
			case 0:
				result = 0.1f;
				break;

			case 1:
				result = 0.3f;
				break;

			case 2:
				result = 0.5f;
				break;

			case 3:
				result = 0.7f;
				break;

			case 4:
				result = 0.9f;
				break;

			default:
				break;
			}
		}
		else
		{
			switch (confidence)
			{
			case 0:
				result = 1.25f;
				break;

			case 1:
				result = 1.0f;
				break;

			case 2:
				result = 0.75f;
				break;

			case 3:
				result = 0.5f;
				break;

			case 4:
				result = 0.25f;
				break;

			default:
				break;
			}
		}
		return result;
	}
	

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

	RE::BSEventNotifyControl animEventHandler::HookedProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* src)
	{
		FnProcessEvent fn = fnHash.at(*(uint64_t*)this);

		if (!a_event.holder) {
			return fn ? (this->*fn)(a_event, src) : RE::BSEventNotifyControl::kContinue;
		}

		RE::Actor* actor = const_cast<RE::TESObjectREFR*>(a_event.holder)->As<RE::Actor>();
		switch (hash(a_event.tag.c_str(), a_event.tag.size())) {
		case "BeginCastLeft"_h:
		    
			break;

		default:
			break;
		}

		return fn ? (this->*fn)(a_event, src) : RE::BSEventNotifyControl::kContinue;
	}

	std::unordered_map<uint64_t, animEventHandler::FnProcessEvent> animEventHandler::fnHash;

	void OnMeleeHitHook::install(){

		auto eventSink = OurEventSink::GetSingleton();

		// ScriptSource
		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		// eventSourceHolder->AddEventSink<RE::TESSwitchRaceCompleteEvent>(eventSink);
		//eventSourceHolder->AddEventSink<RE::TESEquipEvent>(eventSink);
		eventSourceHolder->AddEventSink<RE::TESCombatEvent>(eventSink);
		//eventSourceHolder->AddEventSink<RE::TESActorLocationChangeEvent>(eventSink);
		//eventSourceHolder->AddEventSink<RE::TESSpellCastEvent>(eventSink);
		//eventSourceHolder->AddEventSink<RE::TESDeathEvent>(eventSink);
	}


	void OnMeleeHitHook::install_pluginListener(){
		auto eventSink = OurEventSink::GetSingleton();
		SKSE::GetModCallbackEventSource()->AddEventSink(eventSink);
	}

	InputEventHandler* InputEventHandler::GetSingleton()
	{
		static InputEventHandler singleton;
		return std::addressof(singleton);
	}

	RE::BSEventNotifyControl InputEventHandler::ProcessEvent(RE::InputEvent* const* a_event, [[maybe_unused]] RE::BSTEventSource<RE::InputEvent*>* a_eventSource)
	{
		// using EventType = RE::INPUT_EVENT_TYPE;
		// using DeviceType = RE::INPUT_DEVICE;

		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// for (auto event = *a_event; event; event = event->next) {
		// 	if (event->eventType != EventType::kButton) {
		// 		continue;
		// 	}

		// 	auto button = static_cast<RE::ButtonEvent*>(event);
		// 	if (!button) {
		// 		continue;
		// 	}
		// 	auto key = button->idCode;
		// 	if (!key) {
		// 		continue;
		// 	}

		// 	switch (button->device.get()) {
		// 	case DeviceType::kMouse:
		// 		key += kMouseOffset;
		// 		break;
		// 	case DeviceType::kKeyboard:
		// 		key += kKeyboardOffset;
		// 		break;
		// 	case DeviceType::kGamepad:
		// 		key = GetGamepadIndex((RE::BSWin32GamepadDevice::Key)key);
		// 		break;
		// 	default:
		// 		continue;
		// 	}

		// 	if (key == 274)
		// 	{
		// 		auto Playerhandle = RE::PlayerCharacter::GetSingleton();
		// 		if (button->IsDown() || button->IsHeld() || button->IsPressed()){
		// 			Playerhandle->SetGraphVariableBool("bPSV_Toggle_PowerSprintAttack", true);
		// 		}
		// 		if(button->IsUp()){
		// 			Playerhandle->SetGraphVariableBool("bPSV_Toggle_PowerSprintAttack", false);
		// 		}
		// 		break;
		// 	}
		// }

		return RE::BSEventNotifyControl::kContinue;
	}

	std::uint32_t InputEventHandler::GetGamepadIndex(RE::BSWin32GamepadDevice::Key a_key)
	{
		using Key = RE::BSWin32GamepadDevice::Key;

		std::uint32_t index;
		switch (a_key) {
		case Key::kUp:
			index = 0;
			break;
		case Key::kDown:
			index = 1;
			break;
		case Key::kLeft:
			index = 2;
			break;
		case Key::kRight:
			index = 3;
			break;
		case Key::kStart:
			index = 4;
			break;
		case Key::kBack:
			index = 5;
			break;
		case Key::kLeftThumb:
			index = 6;
			break;
		case Key::kRightThumb:
			index = 7;
			break;
		case Key::kLeftShoulder:
			index = 8;
			break;
		case Key::kRightShoulder:
			index = 9;
			break;
		case Key::kA:
			index = 10;
			break;
		case Key::kB:
			index = 11;
			break;
		case Key::kX:
			index = 12;
			break;
		case Key::kY:
			index = 13;
			break;
		case Key::kLeftTrigger:
			index = 14;
			break;
		case Key::kRightTrigger:
			index = 15;
			break;
		default:
			index = kInvalid;
			break;
		}

		return index != kInvalid ? index + kGamepadOffset : kInvalid;
	}

	void InputEventHandler::SinkEventHandlers()
	{
		auto deviceManager = RE::BSInputDeviceManager::GetSingleton();
		deviceManager->AddEventSink(InputEventHandler::GetSingleton());
		logger::info("Added input event sink");
	}

	bool OnMeleeHitHook::BindPapyrusFunctions(VM* vm)
	{
		//vm->RegisterFunction("XXXX", "XXXXX", XXXX);
		return true;
	}

	int OnMeleeHitHook::GenerateRandomInt(int value_a, int value_b)
	{
		std::mt19937 generator(rd());
		std::uniform_int_distribution<int> dist(value_a, value_b);
		return dist(generator);
	}

	float OnMeleeHitHook::GenerateRandomFloat(float value_a, float value_b)
	{
		std::mt19937 generator(rd());
		std::uniform_real_distribution<float> dist(value_a, value_b);
		return dist(generator);
	}
	double OnMeleeHitHook::GenerateRandomDouble(double value_a, double value_b)
	{
		std::mt19937 generator(rd());
		std::uniform_real_distribution<double> dist(value_a, value_b);
		return dist(generator);
	}

	void OnMeleeHitHook::RegisterforUpdate(RE::Actor *a_actor, std::tuple<RE::MagicCaster *, std::chrono::steady_clock::time_point, std::chrono::milliseconds, std::string> data)
	{
		auto itt = _Timer.find(a_actor);
		if (itt == _Timer.end())
		{
			std::vector<std::tuple<RE::MagicCaster *, std::chrono::steady_clock::time_point, std::chrono::milliseconds, std::string>> Hen;
			Hen.push_back(data);
			_Timer.insert({a_actor, Hen});
		}
		else
		{
			itt->second.push_back(data);
		}
	}

	void OnMeleeHitHook::Update(RE::Actor* a_actor, [[maybe_unused]] float a_delta)
	{
		if (a_actor->GetActorRuntimeData().currentProcess && a_actor->GetActorRuntimeData().currentProcess->InHighProcess() && a_actor->Is3DLoaded() && a_actor->IsInCombat()){

			// GetSingleton()->Process_Updates(a_actor, std::chrono::steady_clock::now());
		}
	}

	void OnMeleeHitHook::Process_Updates(RE::Actor *a_actor, std::chrono::steady_clock::time_point time_now)
	{
		if (!a_actor)
		{
			return;
		}

		const auto &it = _Timer.find(a_actor);

		if (it != _Timer.end())
		{
			if (!it->second.empty())
			{
				for (auto data : it->second)
				{
					RE::MagicCaster *caster = nullptr;
					std::chrono::steady_clock::time_point time_initial;
					std::chrono::milliseconds time_required;
					std::string function;
					std::tie(caster, time_initial, time_required, function) = data;

					if (duration_cast<std::chrono::milliseconds>(time_now - time_initial).count() >= time_required.count())
					{
						auto H = RE::TESDataHandler::GetSingleton();
						switch (hash(function.c_str(), function.size()))
						{
						case "Dummy_Update"_h:

							break;

						default:
							break;
						}
						std::vector<std::tuple<RE::MagicCaster *, std::chrono::steady_clock::time_point, std::chrono::milliseconds, std::string>>::iterator position = std::find(it->second.begin(), it->second.end(), data);
						if (position != it->second.end())
						{
							it->second.erase(position);
						}
					}
				}
			}
			else
			{
				_Timer.erase(it);
			}
		}
	}

	// void OnMeleeHitHook::init()
	// {
	// 	_precision_API = reinterpret_cast<PRECISION_API::IVPrecision1*>(PRECISION_API::RequestPluginAPI());
	// 	if (_precision_API) {
	// 		_precision_API->AddPostHitCallback(SKSE::GetPluginHandle(), PrecisionWeaponsCallback_Post);
	// 		logger::info("Enabled compatibility with Precision");
	// 	}
	// }

	// void OnMeleeHitHook::PrecisionWeaponsCallback_Post(const PRECISION_API::PrecisionHitData& a_precisionHitData, const RE::HitData& a_hitdata)
	// {
	// 	if (!a_precisionHitData.target || !a_precisionHitData.target->Is(RE::FormType::ActorCharacter)) {
	// 		return;
	// 	}
	// 	return;
	// }

	void Settings::Load(){
		constexpr auto path = "Data\\SKSE\\Plugins\\Custom.ini";

		CSimpleIniA ini;
		ini.SetUnicode();

		ini.LoadFile(path);

		general.Load(ini);

		include_spells_mods.Load(ini);
		include_spells_keywords.Load(ini);
		exclude_spells_mods.Load(ini);
		exclude_spells_keywords.Load(ini);

		ini.SaveFile(path);
	}

	

	void Settings::General_Settings::Load(CSimpleIniA &a_ini)
	{
		static const char *section = "General_Settings";

		auto DS = GetSingleton();

		DS->general.bWhiteListApproach = a_ini.GetBoolValue(section, "bWhiteListApproach", DS->general.bWhiteListApproach);
		DS->general.bPatchPureDamageSpells = a_ini.GetBoolValue(section, "bPatchPureDamageSpells", DS->general.bPatchPureDamageSpells);

		a_ini.SetBoolValue(section, "bWhiteListApproach", DS->general.bWhiteListApproach, ";If set to true, only the include mods and keywords are considered. Else only the exclude approach is used");
		a_ini.SetBoolValue(section, "bPatchPureDamageSpells", DS->general.bPatchPureDamageSpells, ";If set to false, pure NSV_Magic_Damage aren't patched which may help prevent strange mage behaviour if you experience them. However, Doing this will lead to less spell variety.");
	}

	void Settings::Include_AllSpells_withKeywords::Load(CSimpleIniA &a_ini)
	{
		static const char *section = "Include_AllSpells_withKeywords";

		auto DS = GetSingleton();

		DS->include_spells_keywords.inc_keywords_joined = a_ini.GetValue(section, "inc_keywords", DS->include_spells_keywords.inc_keywords_joined.c_str());

		std::istringstream f(DS->include_spells_keywords.inc_keywords_joined);
		std::string s;
		while (getline(f, s, '|'))
		{
			DS->include_spells_keywords.inc_keywords.push_back(s);
		}

		a_ini.SetValue(section, "inc_keywords", DS->include_spells_keywords.inc_keywords_joined.c_str(), ";Enter keywords for which all associated spells are included. Seperate keywords with | ");
	}

	void Settings::Include_AllSpells_inMods::Load(CSimpleIniA& a_ini){
		static const char* section = "Include_AllSpells_inMods";

		auto DS = GetSingleton();

		DS->include_spells_mods.inc_mods_joined = a_ini.GetValue(section, "inc_mods", DS->include_spells_mods.inc_mods_joined.c_str());

		std::istringstream f(DS->include_spells_mods.inc_mods_joined);
		std::string  s;
		while (getline(f, s, '|')) {
			DS->include_spells_mods.inc_mods.push_back(s);
		}

		a_ini.SetValue(section, "inc_mods", DS->include_spells_mods.inc_mods_joined.c_str(), ";Enter Mod Names of which all spells within are included. Seperate names with | ");
		//
	}

	void Settings::Exclude_AllSpells_withKeywords::Load(CSimpleIniA& a_ini)
	{
		static const char* section = "Exclude_AllSpells_withKeywords";

		auto DS = GetSingleton();

		DS->exclude_spells_keywords.exc_keywords_joined = a_ini.GetValue(section, "exc_keywords", DS->exclude_spells_keywords.exc_keywords_joined.c_str());

		std::istringstream f(DS->exclude_spells_keywords.exc_keywords_joined);
		std::string  s;
		while (getline(f, s, '|')) {
			DS->exclude_spells_keywords.exc_keywords.push_back(s);
		}

		a_ini.SetValue(section, "exc_keywords", DS->exclude_spells_keywords.exc_keywords_joined.c_str(), ";Enter keywords for which all associated spells are excluded. Seperate keywords with | ");
	}

	void Settings::Exclude_AllSpells_inMods::Load(CSimpleIniA &a_ini)
	{
		static const char *section = "Exclude_AllSpells_inMods";

		auto DS = GetSingleton();

		DS->exclude_spells_mods.exc_mods_joined = a_ini.GetValue(section, "exc_mods", DS->exclude_spells_mods.exc_mods_joined.c_str());

		std::istringstream f(DS->exclude_spells_mods.exc_mods_joined);
		std::string s;
		while (getline(f, s, '|'))
		{
			DS->exclude_spells_mods.exc_mods.push_back(s);
		}

		a_ini.SetValue(section, "exc_mods", DS->exclude_spells_mods.exc_mods_joined.c_str(), ";Enter Mod Names of which all spells within are excluded. Seperate names with | ");
		//
	}


}
