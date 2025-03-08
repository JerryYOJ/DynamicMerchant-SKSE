#include "DynamicLC.h"

#include "../../configmanager.h"

#include <xbyak.h>
#include <random>

std::unordered_map<RE::FormID, float> DynamicLC::levelMap;

static float MinNumMult = 0, MaxNumMult = 0, MinLevelMult = 0, MaxLevelMult = 0;

void DynamicLC::Install()
{
	REL::Relocation<std::uintptr_t> hookPoint{ RELOCATION_ID(15889, 16129), 0x217 };
	auto& t = SKSE::GetTrampoline();
	_CalculateCurrentFormList = t.write_call<5>(hookPoint.address(), CalculateCurrentFormList);

	auto&& cfg = ConfigManager::getInstance();

	cfg.HasKey("MinNumMult", "1.0");
	cfg.HasKey("MaxNumMult", "5.0");
	cfg.HasKey("MinLevelMult", "1.0");
	cfg.HasKey("MaxLevelMult", "3.0");

	MinNumMult = std::stof(cfg.GetKey("MinNumMult"));
	MaxNumMult = std::stof(cfg.GetKey("MaxNumMult"));
	MinLevelMult = std::stof(cfg.GetKey("MinLevelMult"));
	MaxLevelMult = std::stof(cfg.GetKey("MaxLevelMult"));

	logger::info("[LoadConfig] minn:{} maxn:{} minl:{} maxl:{}", MinNumMult, MaxNumMult, MinLevelMult, MaxLevelMult);
}

void DynamicLC::CalculateCurrentFormList(RE::TESLeveledList* thiz, std::uint16_t a_level, std::int16_t a_count, RE::BSScrapArray<RE::CALCED_OBJECT>& a_calcedObjects, std::uint32_t a_arg5, bool a_usePlayerLevel)
{
	struct infoGetter : Xbyak::CodeGenerator {
		infoGetter() {
			mov(rax, r14);
			ret();
		}
	}f;
	f.ready();
	RE::InventoryChanges* caller_inv = f.getCode<RE::InventoryChanges * (*)()>()();
	//RE::InventoryChanges* caller_inv = *(RE::InventoryChanges**)((char*)rbp + 0x57 + 0x10);

	auto process = [&] {
		using _GetFormEditorID = const char* (*)(std::uint32_t);
		static auto tweaks = GetModuleHandle(L"po3_Tweaks");
		static auto GetFormEditorID = reinterpret_cast<_GetFormEditorID>(GetProcAddress(tweaks, "GetFormEditorID"));

		RE::FormID id = caller_inv->owner->formID;
		if (caller_inv->owner->Is(RE::FormType::Reference)) id = caller_inv->owner->As<RE::TESObjectREFR>()->GetBaseObject()->formID;

		if (std::string(GetFormEditorID(id)).contains("Merchant")) {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<int> numMult(MinNumMult, MaxNumMult);
			std::uniform_real_distribution<float> levelMult(MinLevelMult, MaxLevelMult);
			
			int num = numMult(gen);
			float level = levelMult(gen);

			levelMap[caller_inv->owner->formID] = level;

			if(a_usePlayerLevel) _CalculateCurrentFormList(thiz, RE::PlayerCharacter::GetSingleton()->GetLevel()* level, a_count* num, a_calcedObjects, a_arg5, false);
			else _CalculateCurrentFormList(thiz, a_level * level, a_count * num, a_calcedObjects, a_arg5, false);

			logger::info("0x{:X}({}) Generated with level:{} count:{}", id, GetFormEditorID(id), a_level* level, a_count* num);
			
			return;
		}

		_CalculateCurrentFormList(thiz, a_level, a_count, a_calcedObjects, a_arg5, a_usePlayerLevel);
	};

	return process();
}
