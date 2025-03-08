#include "DynamicLC.h"

#include <xbyak.h>
#include <random>

std::unordered_map<RE::FormID, float> DynamicLC::levelMap;

void DynamicLC::Install()
{
	REL::Relocation<std::uintptr_t> hookPoint{ RELOCATION_ID(15889, 16129), 0x217 };
	auto& t = SKSE::GetTrampoline();
	_CalculateCurrentFormList = t.write_call<5>(hookPoint.address(), CalculateCurrentFormList);
}

void DynamicLC::CalculateCurrentFormList(RE::TESLeveledList* thiz, std::uint16_t a_level, std::int16_t a_count, RE::BSScrapArray<RE::CALCED_OBJECT>& a_calcedObjects, std::uint32_t a_arg5, bool a_usePlayerLevel)
{
	struct rbpGetter : Xbyak::CodeGenerator {
		rbpGetter() {
			mov(rax, rbp);
			ret();
		}
	}RBP;
	RBP.ready();
	void* rbp = RBP.getCode<void* (*)()>()();
	RE::InventoryChanges* caller_inv = *(RE::InventoryChanges**)((char*)rbp + 0x67);

	using _GetFormEditorID = const char* (*)(std::uint32_t);
	static auto tweaks = GetModuleHandle(L"po3_Tweaks");
	static auto GetFormEditorID = reinterpret_cast<_GetFormEditorID>(GetProcAddress(tweaks, "GetFormEditorID"));

	RE::FormID id = caller_inv->owner->formID;
	if (caller_inv->owner->Is(RE::FormType::Reference)) id = caller_inv->owner->As<RE::TESObjectREFR>()->GetBaseObject()->formID;

	if (std::string(GetFormEditorID(id)).contains("Merchant")) {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> numMult(1, 5);
		std::uniform_real_distribution<float> levelMult(1.0, 3.0);
		
		int num = numMult(gen);
		float level = levelMult(gen);

		levelMap[caller_inv->owner->formID] = level;

		if(a_usePlayerLevel) _CalculateCurrentFormList(thiz, RE::PlayerCharacter::GetSingleton()->GetLevel()* level, a_count* num, a_calcedObjects, a_arg5, false);
		else _CalculateCurrentFormList(thiz, a_level * level, a_count * num, a_calcedObjects, a_arg5, false);

		logger::info("{}({}) Generated with level:{} count:{}", caller_inv->owner->formID, caller_inv->owner->GetName(), a_level* level, a_count* num);
		
		return;
	}

	return _CalculateCurrentFormList(thiz, a_level, a_count, a_calcedObjects, a_arg5, a_usePlayerLevel);
}
