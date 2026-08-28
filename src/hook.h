#include "PrecisionAPI.h"
#include "SKSE/Trampoline.h"
#include <SimpleIni.h>
#include <iterator>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include <thread>
#include <MinHook.h>
#include "ClibUtil/editorID.hpp"
#pragma warning(disable: 4100)
#pragma warning(disable : 4189)
//using std::string;
static float& g_deltaTime = (*(float*)RELOCATION_ID(523660, 410199).address());


namespace hooks
{
	// static float& g_deltaTime = (*(float*)RELOCATION_ID(523660, 410199).address());
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;
#define STATIC_ARGS [[maybe_unused]] VM *a_vm, [[maybe_unused]] StackID a_stackID, RE::StaticFunctionTag *
#define PI 3.14159265358979323846f

	using EventResult = RE::BSEventNotifyControl;

	union ConditionParam
	{
		char c;
		std::int32_t i;
		float f;
		RE::TESForm *form;
	};

	bool GetshouldHelp(const RE::Actor *p_ally, const RE::Actor *a_actor);
	
	bool IsValidLifeState(RE::Actor *a_actor, bool checkDeath = false);

	int GetPlayerFollowerCount();
	bool IsInSpecialCombatState(RE::Actor *a_actor);

	enum class Difficulty : std::uint32_t
	{
		Novice = 0,		// Novice
		Apprentice = 1, // Apprentice
		Adept = 2,		// Adept
		Expert = 3,		// Expert
		Master = 4,		// Master
		Legendary = 5,	// Legendary
	};

	struct DifficultyState
	{
		Difficulty currentlevel = Difficulty::Novice;

		Difficulty BasePreference = Difficulty::Adept;

	}difficulty;

	void AdjustDifficulty(Difficulty level);
	void SetBaseDifficulty(int level);

	class DGD
	{
	public:

		static DGD* GetSingleton()
		{
			static DGD avInterface;
			return &avInterface;
		}

		static void install();

		int GenerateRandomInt(int value_a, int value_b);
	    float GenerateRandomFloat(float value_a, float value_b);
		double GenerateRandomDouble(double value_a, double value_b);
		static bool GetBoolVariable(RE::Actor *a_actor, std::string a_string);
		static int GetIntVariable(RE::Actor *a_actor, std::string a_string);
		static float GetFloatVariable(RE::Actor *a_actor, std::string a_string);

	private:
		DGD() = default;
		DGD(const DGD&) = delete;
		DGD(DGD&&) = delete;
		~DGD() = default;

		DGD& operator=(const DGD&) = delete;
		DGD& operator=(DGD&&) = delete;

		std::random_device rd;

	protected:

	};

	class Settings
	{
	public:
		static Settings* GetSingleton()
		{
			static Settings avInterface;
			return &avInterface;
		}

		void Load();

		struct General_Settings
		{
			void Load(CSimpleIniA &a_ini);
			void LoadSettings();

			int iBasePreference = 2;
			int iDiffIncrement = 1;

			double fDiffMultHPByPCVE = 1.5;
			double fDiffMultHPByPCE = 1.25;
			double fDiffMultHPByPCN = 1.0;
			double fDiffMultHPByPCH = 0.75;
			double fDiffMultHPByPCVH = 0.5;
			double fDiffMultHPByPCL = 0.25;

			double fDiffMultHPToPCVE = 0.66;
			double fDiffMultHPToPCE = 0.83;
			double fDiffMultHPToPCN = 1.0;
			double fDiffMultHPToPCH = 1.17;
			double fDiffMultHPToPCVH = 1.34;
			double fDiffMultHPToPCL = 1.51;

		} general;

	private:
		Settings() = default;
		Settings(const Settings&) = delete;
		Settings(Settings&&) = delete;
		~Settings() = default;

		Settings& operator=(const Settings&) = delete;
		Settings& operator=(Settings&&) = delete;
	};
};

constexpr uint32_t hash(const char* data, size_t const size) noexcept
{
	uint32_t hash = 5381;

	for (const char* c = data; c < data + size; ++c) {
		hash = ((hash << 5) + hash) + (unsigned char)*c;
	}

	return hash;
}

constexpr uint32_t operator"" _h(const char* str, size_t size) noexcept
{
	return hash(str, size);
}
