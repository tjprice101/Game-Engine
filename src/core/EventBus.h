#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <algorithm>

// ---- Type-safe publish/subscribe event bus ----------------------------------
// Usage:
//   EventBus::on<PlayerJumpEvent>([](const PlayerJumpEvent& e){ ... });
//   EventBus::emit(PlayerJumpEvent{ .height = 5.f });

class EventBus {
public:
    template<typename T>
    using Handler = std::function<void(const T&)>;

    template<typename T>
    static int on(Handler<T> handler) {
        auto& list = handlers<T>();
        int id = s_nextId++;
        list.push_back({ id, std::move(handler) });
        return id; // return handle for unsubscribing
    }

    template<typename T>
    static void off(int handleId) {
        auto& list = handlers<T>();
        list.erase(std::remove_if(list.begin(), list.end(),
            [handleId](const Entry<T>& e){ return e.id == handleId; }),
            list.end());
    }

    template<typename T>
    static void emit(const T& event) {
        for (auto& entry : handlers<T>())
            entry.handler(event);
    }

private:
    template<typename T>
    struct Entry { int id; Handler<T> handler; };

    template<typename T>
    static std::vector<Entry<T>>& handlers() {
        static std::vector<Entry<T>> list;
        return list;
    }

    static inline int s_nextId = 0;
};

// ---- Standard engine events -------------------------------------------------
struct EvTileChanged   { int tx, ty; };
struct EvPlayerDamage  { int amount; };
struct EvPlayerDeath   {};
struct EvChunkLoaded   { int cx, cy; };
struct EvScreenResize  { int w, h; };
struct EvDayNightCycle { float dayTime; float sunlight; };
