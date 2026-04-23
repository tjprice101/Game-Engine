#include "game/Game.h"
#include <iostream>
#include <memory>

int main() {
    try {
        WindowConfig cfg;
        cfg.width     = 1280;
        cfg.height    = 720;
        cfg.title     = "Block Engine";
        cfg.vsync     = true;
        cfg.resizable = true;

        auto game = std::make_unique<Game>(cfg);
        game->run();
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << '\n';
        return 1;
    }
    return 0;
}
