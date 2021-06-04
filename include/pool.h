#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

class BuddyHeap
{
public:
    BuddyHeap(size_t min_p, size_t max_p);

    size_t & operator[](size_t);

    size_t operator[](size_t) const;

    size_t get_available_space(size_t) const;

    size_t get_available_segment(size_t) const;

    std::optional<size_t> get_index_of_used_segment(size_t) const;

    void update_state(size_t);
    void merge(size_t);

    const size_t min_p;
    const size_t max_p;

private:
    inline static size_t get_parent_index(size_t);
    inline static size_t left_child_index(size_t);
    inline static size_t right_child_index(size_t);

    std::vector<size_t> tree;
};

class PoolAllocator
{
public:
    PoolAllocator(size_t min_p, size_t max_p);

    void * allocate(std::size_t n);

    void deallocate(const void * ptr);

private:
    std::vector<std::byte> m_pool;
    BuddyHeap m_heap;
};
