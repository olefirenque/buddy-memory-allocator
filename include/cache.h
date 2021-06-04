#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <iostream>
#include <new>
#include <ostream>

template <class Key, class KeyProvider, class Allocator>
class Cache
{
public:
    template <class... AllocArgs>
    Cache(const std::size_t cache_size, AllocArgs &&... alloc_args)
        : m_max_size(cache_size)
        , m_alloc(std::forward<AllocArgs>(alloc_args)...)
    {
    }

    struct CachedElement
    {
        CachedElement(KeyProvider * obj, bool usability)
            : keyProvider(obj)
            , isUsed(usability)
        {
        }

        KeyProvider * keyProvider;
        bool isUsed;
    };

    std::size_t size() const
    {
        return m_cache.size();
    }

    bool empty() const
    {
        return m_cache.empty();
    }

    template <class T>
    T & get(const Key & key);

    std::ostream & print(std::ostream & strm) const;

    friend std::ostream & operator<<(std::ostream & strm, const Cache & cache)
    {
        return cache.print(strm);
    }

private:
    const std::size_t m_max_size;
    Allocator m_alloc;
    std::deque<CachedElement> m_cache;
};

template <class Key, class KeyProvider, class Allocator>
template <class T>
inline T & Cache<Key, KeyProvider, Allocator>::get(const Key & key)
{
    auto it = std::find_if(m_cache.begin(), m_cache.end(), [&key](CachedElement & elem) {
        return *static_cast<T *>(elem.keyProvider) == key;
    });
    if (it != m_cache.end()) {
        CachedElement & cachedElement = *it;
        cachedElement.isUsed = true;
        return *static_cast<T *>(cachedElement.keyProvider);
    }
    if (m_max_size == m_cache.size()) {
        size_t number_of_deletions = 0;
        for (; number_of_deletions < m_max_size && m_cache.back().isUsed; ++number_of_deletions) {
            m_cache.template emplace_front(m_cache.back());
            m_cache.pop_back();
        }
        if (number_of_deletions != m_max_size) {
            m_cache.front().isUsed = false;
        }
        else {
            for_each(m_cache.begin(), m_cache.end(), [](CachedElement & cachedElement) {
                cachedElement.isUsed = false;
            });
        }
        m_alloc.destroy(m_cache.back().keyProvider);
        m_cache.pop_back();
    }
    m_cache.template emplace_front(CachedElement(m_alloc.template create<T>(T(key)), false));
    return *static_cast<T *>(m_cache.front().keyProvider);
}

template <class Key, class KeyProvider, class Allocator>
inline std::ostream & Cache<Key, KeyProvider, Allocator>::print(std::ostream & strm) const
{
    return strm << "<empty>\n";
}
