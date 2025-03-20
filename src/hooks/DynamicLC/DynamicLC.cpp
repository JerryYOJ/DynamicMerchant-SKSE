#include "DynamicLC.h"

#include "../../configmanager.h"

#include <xbyak.h>
#include <random>

static float MinNumMult = 0, MaxNumMult = 0, MinLevelMult = 0, MaxLevelMult = 0, PriceMult = 0;

void DynamicLC::Install()
{
	struct trampolineAE : Xbyak::CodeGenerator {
		trampolineAE(){
			mov(ptr[rsp + 0x28], r14);
			mov(r11, (uintptr_t)CalculateCurrentFormList);
			jmp(r11);
		}
	};
	struct trampolineSE : Xbyak::CodeGenerator {
		trampolineSE() {
			mov(ptr[rsp + 0x28], r15);
			mov(r11, (uintptr_t)CalculateCurrentFormList);
			jmp(r11);
		}
	};

	auto& t = SKSE::GetTrampoline();

	auto code = [&] {
		if (REL::Module::IsAE()) {
			trampolineAE code;
			return t.allocate(code);
		}
		else if (REL::Module::IsSE()) {
			trampolineSE code;
			return t.allocate(code);
		}
		return (void*)nullptr;
	};

	REL::Relocation<std::uintptr_t> hookPoint{ RELOCATION_ID(15889, 16129), REL::VariantOffset(0x1C1, 0x217, 0x0) };
	
	_CalculateCurrentFormList = t.write_call<5>(hookPoint.address(), code());

	auto&& cfg = ConfigManager::getInstance();

	cfg.HasKey("MinNumMult", "1.0");
	cfg.HasKey("MaxNumMult", "5.0");
	cfg.HasKey("MinLevelMult", "1.0");
	cfg.HasKey("MaxLevelMult", "3.0");
	cfg.HasKey("PriceMult", "1.0");

	MinNumMult = std::stof(cfg.GetKey("MinNumMult"));
	MaxNumMult = std::stof(cfg.GetKey("MaxNumMult"));
	MinLevelMult = std::stof(cfg.GetKey("MinLevelMult"));
	MaxLevelMult = std::stof(cfg.GetKey("MaxLevelMult"));
	PriceMult = std::stof(cfg.GetKey("PriceMult"));

	logger::info("[LoadConfig] minn:{} maxn:{} minl:{} maxl:{} pm:{}", MinNumMult, MaxNumMult, MinLevelMult, MaxLevelMult, PriceMult);
}

void DynamicLC::CalculateCurrentFormList(RE::TESLeveledList* thiz, std::uint16_t a_level, std::int16_t a_count, RE::BSScrapArray<RE::CALCED_OBJECT>& a_calcedObjects, RE::InventoryChanges* caller_inv, bool a_usePlayerLevel)
{
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
		float lvl = levelMult(gen);

		uint16_t level = a_usePlayerLevel ? RE::PlayerCharacter::GetSingleton()->GetLevel() * lvl : a_level * lvl;

		_CalculateCurrentFormList(thiz, level, a_count * num, a_calcedObjects, nullptr, false);

		logger::info("0x{:X}({}) Generated with level:{} count:{}*{}", id, GetFormEditorID(id), level, a_count, num);
			
		return;
	}

	_CalculateCurrentFormList(thiz, a_level, a_count, a_calcedObjects, nullptr, a_usePlayerLevel);

	return;
}


// extern "C" __declspec(dllexport) creates an dll exported function that others can use
// RE::Actor* trader is the merchant's reference
// RE::InventoryEntryData* objDesc is the item player is looking at
// uint16_t a_level is the item's generated level form leveledlists
// RE::GFxValue& a_updateObj is the item's gfx object
// bool is_buying is whether the player is buying item from merchant (true) or selling to the merchant (false)
extern "C" __declspec(dllexport) float MerchantPriceCallback(RE::Actor* trader, RE::InventoryEntryData* objDesc, uint16_t a_level, RE::GFxValue& a_updateObj, bool is_buying) {
	if (is_buying) {
		uint16_t PCL = RE::PlayerCharacter::GetSingleton()->GetLevel();
		if (a_level > PCL) {
			RE::GFxValue n(RE::GFxValue::ValueType::kString);
			a_updateObj.GetMember("name", &n);
			std::string name = n.GetString();
			name += std::format(" (LVL:{})", a_level);
			n.SetString(name);
			a_updateObj.SetMember("name", n);  //Change names to include level info

			return (float)a_level / (float)PCL;
		}
	}
	
	return 1.0f;
}
