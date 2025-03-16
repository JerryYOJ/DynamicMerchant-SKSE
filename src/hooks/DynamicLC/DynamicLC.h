class DynamicLC {

public:
    static void Install();
protected:
    static void CalculateCurrentFormList(RE::TESLeveledList* thiz, std::uint16_t a_level, std::int16_t a_count, RE::BSScrapArray<RE::CALCED_OBJECT>& a_calcedObjects, RE::InventoryChanges* caller_inv, bool a_usePlayerLevel);

private:

    DynamicLC() = delete;
    DynamicLC(const DynamicLC&) = delete;
    DynamicLC(DynamicLC&&) = delete;
    ~DynamicLC() = delete;

    DynamicLC& operator=(const DynamicLC&) = delete;
    DynamicLC& operator=(DynamicLC&&) = delete;

    inline static REL::Relocation<decltype(CalculateCurrentFormList)> _CalculateCurrentFormList;
};