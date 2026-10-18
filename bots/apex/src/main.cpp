// Dispatch draft and gameplay; a failed turn is logged and still yields.
#include "bot.h"
#include "draft.h"

int main() {
    apex::Bot bot;
    while (true) {
        try {
            if (cambc::is_draft())
                apex::play_draft();
            else
                bot.play_turn();
        } catch (const std::exception& error) {
            std::printf("bot error: %s\n", error.what());
        } catch (...) {
            std::printf("bot error: unknown exception\n");
        }
        if (!cambc::is_draft()) bot.end_of_turn();
        cambc::end_turn();
    }
}
