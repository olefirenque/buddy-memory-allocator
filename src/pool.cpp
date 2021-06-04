#include "pool.h"

#include <limits>

using std::size_t;

const size_t UNAVAILABLE_MASK = (std::numeric_limits<std::size_t>::max() >> 1ull) + 1ull; // first bit mask which says if element is used

inline uint32_t log2(uint32_t x)
{
    static const int deBruijnBitPosition[32] = {0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31};
    x |= x >> 1u;
    x |= x >> 2u;
    x |= x >> 4u;
    x |= x >> 8u;
    x |= x >> 16u;
    return deBruijnBitPosition[(x * 0x07C4ACDDU) >> 27u];
}

PoolAllocator::PoolAllocator(const size_t min_p, const size_t max_p)
    : m_pool(1ull << max_p)
    , m_heap(min_p, max_p)
{
}

void * PoolAllocator::allocate(const std::size_t n)
{
    size_t layer = std::max(m_heap.min_p, n == 1 ? 0 : static_cast<size_t>(log2(n - 1u) + 1u));
    size_t pool_index = m_heap.get_available_segment(layer);
    m_heap[pool_index] |= UNAVAILABLE_MASK;
    size_t offset = (pool_index + 1ull - (1ull << (m_heap.max_p - layer))) << layer;
    m_heap.update_state(pool_index);
    return static_cast<void *>(&m_pool[offset]);
}

void PoolAllocator::deallocate(const void * ptr)
{
    size_t offset = static_cast<const std::byte *>(ptr) - m_pool.data();
    size_t tree_index = ((offset + m_pool.size()) >> m_heap.min_p) - 1u;
    auto usedSegment = m_heap.get_index_of_used_segment(tree_index);
    if (!usedSegment) {
        return; // if top reached and used segment isn't found
    }
    tree_index = usedSegment.value();
    m_heap[tree_index] &= ~UNAVAILABLE_MASK;
    m_heap.merge(tree_index);
}

BuddyHeap::BuddyHeap(size_t min_p, size_t max_p)
    : min_p(min_p)
    , max_p(max_p)
{
    tree.reserve((1ull << (max_p - min_p + 1ull)) - 1ull);
    size_t nodeLayer = max_p;
    for (size_t i = 0u; i <= max_p - min_p; ++i, --nodeLayer) {
        for (size_t j = 0u; j < (1ull << i); ++j) {
            tree.emplace_back(nodeLayer);
        }
    }
}

void BuddyHeap::update_state(size_t index)
{
    while (index) {
        index = get_parent_index(index);
        size_t left = get_available_space(left_child_index(index));
        size_t right = get_available_space(right_child_index(index));
        tree[index] = std::max(left, right);
    }
}

void BuddyHeap::merge(size_t index)
{
    size_t layer = tree[index];
    while (index != 0u) {
        index = get_parent_index(index);
        size_t left = get_available_space(left_child_index(index));
        size_t right = get_available_space(right_child_index(index));
        if (left == layer && right == layer) {
            tree[index] = layer + 1u;
        }
        else {
            tree[index] = std::max(left, right);
        }
        ++layer;
    }
}

size_t BuddyHeap::get_available_segment(size_t layer) const
{
    if (tree[0] >= UNAVAILABLE_MASK || tree[0] < layer) {
        throw std::bad_alloc();
    }
    size_t index = 0;
    for (size_t nodeLayer = max_p; nodeLayer != layer; --nodeLayer) {
        size_t leftIndex = left_child_index(index);
        size_t rightIndex = right_child_index(index);
        size_t min;
        if (tree[leftIndex] <= tree[rightIndex]) {
            if (tree[leftIndex] >= layer) {
                min = leftIndex;
            }
            else {
                min = rightIndex;
            }
        }
        else {
            if (tree[rightIndex] >= layer) {
                min = rightIndex;
            }
            else {
                min = leftIndex;
            }
        }

        if (tree[min] >= layer) {
            index = min;
        }
    }
    return index;
}

std::optional<size_t> BuddyHeap::get_index_of_used_segment(size_t start_index) const
{
    for (size_t layer = min_p; tree[start_index] < UNAVAILABLE_MASK; ++layer) {
        if (start_index == 0u) {
            return {};
        }
        start_index = get_parent_index(start_index);
    }
    return start_index;
}

size_t BuddyHeap::get_available_space(size_t index) const
{
    return tree[index] < UNAVAILABLE_MASK ? tree[index] : 0;
}

size_t BuddyHeap::get_parent_index(size_t index)
{
    return ((index - 1) / 2);
}

size_t BuddyHeap::left_child_index(size_t index)
{
    return (index * 2) + 1;
}

size_t BuddyHeap::right_child_index(size_t index)
{
    return (index * 2) + 2;
}

size_t & BuddyHeap::operator[](size_t index)
{
    return tree[index];
}

size_t BuddyHeap::operator[](size_t index) const
{
    return tree[index];
}
