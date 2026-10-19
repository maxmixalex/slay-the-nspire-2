// Original-run state is appended after the historical expedition fields.
// Legacy save readers never reinterpret their xorshift streams as these RNGs.
struct WorldState {
    uint8_t active=0,rng_ready=0,act_index=0,ascension=0;
    uint16_t total_floor=0,rooms_completed=0,combat_count=0;
    int16_t map_column=0,map_row=0;
    uint64_t run_seed=0;
    char seed_text[65]{};
    reference::Random rng[WorldRngCount]{};
    uint8_t room_type=0,current_point=reference::NoPoint;
    uint8_t map_ready[3]{};
    reference::ReferenceMap maps[3]{};
};
