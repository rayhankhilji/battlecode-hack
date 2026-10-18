// Probe bot: dumps engine metadata in the draft and measures SDK fuel costs in play.
// Keeps the starter skeleton: a failed turn is logged and still yields.
#include <cambc.h>
#include <iostream>
#include "bot.h"
#include "draft.h"
#include "out.h"

int main() {
    Bot bot;
    while (true) {
        try {
            if (cambc::is_draft())
                play_draft();
            else
                bot.play_turn();
        } catch (const cambc::error& error) {
            std::cout << "bot error: " << cambc::to_string(error.code()) << ": " << error.message() << std::endl;
        } catch (const std::exception& error) {
            std::cout << "bot error: " << error.what() << std::endl;
        } catch (...) {
            std::cout << "bot error: unknown exception" << std::endl;
        }

        out::flush();
        cambc::end_turn();
    }
}
