extern class DynamicLC;
extern class StoreVendorFaction;

class DynamicPrices : public RE::GFxFunctionHandler {
public:
	DynamicPrices(const RE::GFxValue& old) : oldFunc{old} {}

	void Call(Params& a_params) override {
		static REL::Relocation<RE::RefHandle*> TraderRefhandle{ RELOCATION_ID(519283, 405823) };

		auto&& traderRef = *TraderRefhandle;
		auto&& trader = RE::TESObjectREFR::LookupByHandle(traderRef);

		if (trader) {
			RE::TESFaction* vendorfact = nullptr;
			for (int cycle = 1; cycle <= 10 && vendorfact == nullptr; cycle++, vendorfact = trader->As<RE::Actor>()->GetVendorFaction()); //weird
			if (vendorfact && vendorfact->vendorData.merchantContainer) {
				auto level = DynamicLC::levelMap[vendorfact->vendorData.merchantContainer->formID];
				
				if (level >= 1.0 && a_params.argCount >= 2) {
					auto&& buy = a_params.args[0];
					//auto&& sell = a_params.args[1];

					buy.SetNumber(buy.GetNumber() * level);
				}
			}
		}

		oldFunc.Invoke("call", a_params.retVal, a_params.argsWithThisRef, a_params.argCount + 1);
	}

	static void Install() {
		
		REL::Relocation<std::uintptr_t> Vtbl{ RE::VTABLE_BarterMenu[0]};
		_PostCreate = Vtbl.write_vfunc(0x2, &PostCreate);

		logger::info("Hooked bartermenu");
	}
private:
	static void PostCreate(RE::BarterMenu* thiz) {
		_PostCreate(thiz);

		auto&& barter = thiz;
		auto&& root = barter->GetRuntimeData().root;

		RE::GFxValue oldf;
		root.GetMember("SetBarterMultipliers", &oldf);

		RE::GFxValue newf;
		auto&& impl = RE::make_gptr<DynamicPrices>(oldf);
		barter->uiMovie->CreateFunction(&newf, impl.get());

		root.SetMember("SetBarterMultipliers", newf);
	}
	static inline REL::Relocation<decltype(&RE::BarterMenu::PostCreate)> _PostCreate;

	RE::GFxValue oldFunc;
};