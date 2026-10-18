// Minimal bot without <iostream>: measures fuel left at the start of the first turns (static-init cost).
#include <cambc.h>
#include <cstdio>
int main() {
    int turns = 0;
    while (true) {
        try {
            const auto fuel = cambc::fuel_remaining();
            if (++turns <= 3) std::printf("M draft=%d turn=%d fuel_at_start=%llu\n", cambc::is_draft() ? 1 : 0, turns, static_cast<unsigned long long>(fuel));
            if (cambc::is_draft()) {
                const auto chosen = cambc::draft::current_draft(cambc::team::ally);
                for (int i = 0; i < 11; ++i) {
                    const auto pick = static_cast<cambc::champion_draft>(i);
                    bool taken = false;
                    for (auto c : chosen) taken = taken || c == pick;
                    if (!taken) { cambc::draft::pick(pick); break; }
                }
            }
        } catch (...) { std::printf("M error\n"); }
        cambc::end_turn();
    }
}
