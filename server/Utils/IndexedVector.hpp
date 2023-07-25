#pragma once

#include <assert.h>
#include <vector>
#include <unordered_map>
#include <optional>
#include <type_traits>

namespace utils {

template<typename Item, typename IndexKey>
class IndexedVector
{
    using HashFunc = std::function<IndexKey(const Item&)>;

    HashFunc          m_fHash;
    std::vector<Item> m_items;
    std::unordered_map<IndexKey, size_t> m_index;

public:
    using Predicate = std::function<bool(const Item& item)>;

    IndexedVector(HashFunc fHash) : m_fHash(fHash) {}

    void push(Item item) {
        m_index[m_fHash(item)] = m_items.size();
        m_items.emplace_back(std::move(item));
    }

    // Takes amortized O(1)
    bool has(const IndexKey& index) const {
        return m_index.find(index) != m_index.end();
    }

    std::vector<Item>&       items() { return m_items; }
    const std::vector<Item>& items() const { return m_items; }

    // Takes O(N)
    bool has(const Predicate& predicate) const {
        for (const Item& item: m_items) {
            if (predicate(item)) {
                return true;
            }
        }
        return false;
    }

    size_t size() const { return m_items.size(); }

    // Takes amortized O(1)
    Item* get(const IndexKey& index) {
        const auto it = m_index.find(index);
        if (it != m_index.end()) {
            const size_t id = it->second;
            if (id < m_index.size()) {
                return &m_items[id];
            } else {
                assert(!"Inconsistent state");
            }
        }
        return nullptr;
    }

    const Item* get(const IndexKey& index) const {
        return const_cast<typename std::remove_const<decltype(this)>::type>(this)->get(index);
    }

    bool pop(const IndexKey& index, Item& dest) {
        auto it = m_index.find(index);
        if (it != m_index.end()) {
            const size_t id = it->second;
            if (id < m_items.size()) {
                dest = std::move(m_items[id]);
                m_index.erase(it);
                removeItem(id);
                return true;
            } else {
                assert(!"Inconsistent state");
            }
        }
        return false;
    }

    // Remove all items that match the specified 'predicate'
    size_t remove(const Predicate& predicate) {
        size_t nRemovedCounter = 0;
        for (ssize_t id = m_items.size() - 1; id >= 0; --id) {
            const Item& item = m_items[id];
            if (predicate(item)) {
                m_index.erase(m_fHash(item));
                removeItem(id);
                ++nRemovedCounter;
            }
        }
        return nRemovedCounter;
    }

    bool remove(const IndexKey& key) {
        auto I = m_index.find(key);
        if (I != m_index.end()) { // [[likely]]
            removeItem(I->second);
            m_index.erase(I);
            return true;
        }
        return false;
    }

private:
    void removeItem(size_t id) {
        if (id < m_items.size() - 1) {
            std::swap(m_items[id], m_items.back());
            const IndexKey index = m_fHash(m_items[id]);
            m_index[index] = id;
        }
        m_items.pop_back();
    }

};

}  // namespace utils