#pragma once
#include <cstdint>
#include <cassert>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <bitset>
#include <type_traits>

// ---- Entity ID ---------------------------------------------------------------
// Layout: [gen:8 | idx:24]  — generation allows safe handle reuse detection
using EntityID = uint32_t;
static constexpr EntityID NULL_ENTITY = 0xFFFFFFFF;

static inline uint32_t entityIndex(EntityID e)      { return e & 0x00FFFFFFu; }
static inline uint8_t  entityGeneration(EntityID e) { return (e >> 24) & 0xFF; }
static inline EntityID makeEntity(uint32_t idx, uint8_t gen) {
    return ((uint32_t)gen << 24) | (idx & 0x00FFFFFFu);
}

// ---- Component type ID -------------------------------------------------------
namespace detail {
    inline uint32_t nextComponentID() {
        static uint32_t counter = 0;
        return counter++;
    }
    template<typename T>
    inline uint32_t componentID() {
        static uint32_t id = nextComponentID();
        return id;
    }
}

static constexpr int MAX_COMPONENTS = 64;
using ComponentMask = std::bitset<MAX_COMPONENTS>;

// ---- ComponentPool -----------------------------------------------------------
// Dense array + sparse reverse-index for O(1) add/remove/get
struct IComponentPool {
    virtual ~IComponentPool() = default;
    virtual void remove(uint32_t idx) = 0;
    virtual bool has(uint32_t idx) const = 0;
};

template<typename T>
class ComponentPool : public IComponentPool {
public:
    T& add(uint32_t entityIdx, T&& val = {}) {
        assert(!has(entityIdx) && "Component already added");
        if (entityIdx >= m_sparse.size()) m_sparse.resize(entityIdx + 1, UINT32_MAX);
        m_sparse[entityIdx] = (uint32_t)m_dense.size();
        m_entityIndex.push_back(entityIdx);
        m_dense.push_back(std::move(val));
        return m_dense.back();
    }

    T& get(uint32_t entityIdx) {
        assert(has(entityIdx));
        return m_dense[m_sparse[entityIdx]];
    }

    const T& get(uint32_t entityIdx) const {
        assert(has(entityIdx));
        return m_dense[m_sparse[entityIdx]];
    }

    void remove(uint32_t entityIdx) override {
        if (!has(entityIdx)) return;
        uint32_t denseIdx = m_sparse[entityIdx];
        uint32_t last = (uint32_t)m_dense.size() - 1;
        if (denseIdx != last) {
            // Swap with last
            m_dense[denseIdx]       = std::move(m_dense[last]);
            m_entityIndex[denseIdx] = m_entityIndex[last];
            m_sparse[m_entityIndex[denseIdx]] = denseIdx;
        }
        m_sparse[entityIdx] = UINT32_MAX;
        m_dense.pop_back();
        m_entityIndex.pop_back();
    }

    bool has(uint32_t entityIdx) const override {
        return entityIdx < m_sparse.size() && m_sparse[entityIdx] != UINT32_MAX;
    }

    // Iteration over all components
    auto begin() { return m_dense.begin(); }
    auto end()   { return m_dense.end();   }
    uint32_t entityAt(size_t denseIdx) const { return m_entityIndex[denseIdx]; }
    size_t   size()                    const { return m_dense.size(); }

private:
    std::vector<T>        m_dense;
    std::vector<uint32_t> m_entityIndex; // dense → entityIdx
    std::vector<uint32_t> m_sparse;      // entityIdx → denseIdx
};

// ---- EntityManager -----------------------------------------------------------
class EntityManager {
public:
    EntityManager() {
        // Reserve slot 0 as tombstone
        m_generations.push_back(0);
        m_freeList.push_back(0);
    }

    EntityID create() {
        uint32_t idx;
        if (m_freeList.size() > 1) {
            idx = m_freeList.back();
            m_freeList.pop_back();
        } else {
            idx = (uint32_t)m_generations.size();
            m_generations.push_back(0);
            if (idx >= m_masks.size()) m_masks.resize(idx + 1);
        }
        EntityID e = makeEntity(idx, m_generations[idx]);
        m_alive.push_back(e);
        return e;
    }

    void destroy(EntityID e) {
        uint32_t idx = entityIndex(e);
        if (!isAlive(e)) return;

        // Remove all components
        m_masks[idx].reset();
        for (auto& [cid, pool] : m_pools) {
            if (pool->has(idx)) pool->remove(idx);
        }

        // Increment generation to invalidate old handles
        ++m_generations[idx];
        m_freeList.push_back(idx);

        // Remove from alive list
        auto it = std::find(m_alive.begin(), m_alive.end(), e);
        if (it != m_alive.end()) m_alive.erase(it);
    }

    bool isAlive(EntityID e) const {
        uint32_t idx = entityIndex(e);
        return idx < m_generations.size()
            && m_generations[idx] == entityGeneration(e);
    }

    template<typename T>
    T& add(EntityID e, T val = {}) {
        uint32_t idx = entityIndex(e);
        uint32_t cid = detail::componentID<T>();
        ensurePool<T>(cid);
        m_masks[idx].set(cid);
        return pool<T>()->add(idx, std::move(val));
    }

    template<typename T>
    T& get(EntityID e) {
        return pool<T>()->get(entityIndex(e));
    }

    template<typename T>
    const T& get(EntityID e) const {
        return pool<T>()->get(entityIndex(e));
    }

    template<typename T>
    bool has(EntityID e) const {
        uint32_t cid = detail::componentID<T>();
        if (cid >= MAX_COMPONENTS) return false;
        uint32_t idx = entityIndex(e);
        if (idx >= m_masks.size()) return false;
        return m_masks[idx].test(cid);
    }

    template<typename T>
    void remove(EntityID e) {
        uint32_t cid = detail::componentID<T>();
        uint32_t idx = entityIndex(e);
        m_masks[idx].reset(cid);
        pool<T>()->remove(idx);
    }

    // Iterate all entities with ALL listed components
    // Usage: view<CTransform, CSprite>([](EntityID e, CTransform& t, CSprite& s){...});
    template<typename... Ts, typename Func>
    void view(Func&& fn) {
        ComponentMask required;
        (required.set(detail::componentID<Ts>()), ...);

        for (EntityID e : m_alive) {
            uint32_t idx = entityIndex(e);
            if (idx >= m_masks.size()) continue;
            if ((m_masks[idx] & required) == required) {
                fn(e, get<Ts>(e)...);
            }
        }
    }

    // Single-component fast iteration (iterates dense array directly)
    template<typename T, typename Func>
    void each(Func&& fn) {
        auto* p = pool<T>();
        if (!p) return;
        for (size_t i = 0; i < p->size(); ++i) {
            EntityID e = makeEntity(p->entityAt(i), m_generations[p->entityAt(i)]);
            fn(e, (*p->begin() + i));
        }
    }

    size_t entityCount() const { return m_alive.size(); }
    const std::vector<EntityID>& alive() const { return m_alive; }

private:
    template<typename T>
    void ensurePool(uint32_t cid) {
        if (m_pools.find(cid) == m_pools.end()) {
            m_pools[cid] = std::make_unique<ComponentPool<T>>();
        }
    }

    template<typename T>
    ComponentPool<T>* pool() {
        uint32_t cid = detail::componentID<T>();
        auto it = m_pools.find(cid);
        return it != m_pools.end()
            ? static_cast<ComponentPool<T>*>(it->second.get())
            : nullptr;
    }

    template<typename T>
    const ComponentPool<T>* pool() const {
        uint32_t cid = detail::componentID<T>();
        auto it = m_pools.find(cid);
        return it != m_pools.end()
            ? static_cast<const ComponentPool<T>*>(it->second.get())
            : nullptr;
    }

    std::vector<uint8_t>     m_generations;
    std::vector<uint32_t>    m_freeList;
    std::vector<ComponentMask> m_masks;
    std::vector<EntityID>    m_alive;
    std::unordered_map<uint32_t, std::unique_ptr<IComponentPool>> m_pools;
};
