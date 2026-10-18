// Dispatch draft and gameplay; a failed turn is logged and still yields.
// Telemetry: one line per champion every 100 rounds, plus SUSPENDED when a
// fuel suspension made the round counter skip between two loop iterations.
#include <cambc.h>
#include <cstdint>
#include <cstdio>
#include <exception>
#include "bot.h"
#include "draft.h"

int main() {
    Bot bot;
    bool have_round = false;
    std::uint32_t last_round = 0;
    std::uint64_t fuel_min = ~std::uint64_t{0};
    while (true) {
        try {
            if (cambc::is_draft()) {
                play_draft();
            } else {
                const auto round = cambc::round_number();
                if (have_round && round > last_round + 1)
                    std::printf("SUSPENDED r=%u\n", static_cast<unsigned>(round));
                have_round = true;
                last_round = round;
                bot.play_turn();
                const auto fuel = cambc::fuel_remaining();
                if (fuel < fuel_min) fuel_min = fuel;
                if (round % 100 == 0) {
                    bot.telemetry(fuel_min);
                    fuel_min = ~std::uint64_t{0};
                }
            }
        } catch (const std::exception& error) {
            std::printf("bot error r=%u: %s\n", static_cast<unsigned>(cambc::round_number()), error.what());
        } catch (...) {
            std::printf("bot error: unknown exception\n");
        }
        cambc::end_turn();
    }
}
