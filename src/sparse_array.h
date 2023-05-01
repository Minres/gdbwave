/*
Copyright 2023 MINRES Technologies GmbH

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _SPARSE_ARRAY_H_
#define _SPARSE_ARRAY_H_

#include <array>
#include <cassert>

/**
 *  @brief a sparse array suitable for large sizes
 *
 *  a simple array which allocates memory in configurable chunks (size of 2^PAGE_ADDR_BITS), used for
 *  large sparse arrays. Memory is allocated on demand
 */
template <typename T, uint64_t SIZE, unsigned PAGE_ADDR_BITS = 24> class sparse_array {
public:
    static_assert(SIZE>0, "sparse_array size must be greater than 0");

    const uint64_t page_addr_mask = (1 << PAGE_ADDR_BITS) - 1;

    const uint64_t page_size = (1 << PAGE_ADDR_BITS);

    const unsigned page_count = (SIZE + page_size - 1) / page_size;

    const uint64_t page_addr_width = PAGE_ADDR_BITS;

    using page_type = std::array<T, 1 << PAGE_ADDR_BITS>;
    /**
     * the default constructor
     */
    sparse_array() { arr.fill(nullptr); }
    /**
     * the destructor
     */
    ~sparse_array() {
        for(auto i : arr)
            delete i;
    }
    /**
     * element access operator
     *
     * @param addr address to access
     * @return the data type reference
     */
    T& operator[](uint32_t addr) {
        assert(addr < SIZE);
        T nr = addr >> PAGE_ADDR_BITS;
        if(arr[nr] == nullptr)
            arr[nr] = new page_type();
        return arr[nr]->at(addr & page_addr_mask);
    }
    /**
     * page fetch operator
     *
     * @param page_nr the page number ot fetch
     * @return reference to page
     */
    page_type& operator()(uint32_t page_nr) {
        assert(page_nr < page_count);
        if(arr[page_nr] == nullptr)
            arr.at(page_nr) = new page_type();
        return *(arr[page_nr]);
    }
    /**
     * check if page for address is allocated
     *
     * @param addr the address to check
     * @return true if the page is allocated
     */
    bool is_allocated(uint32_t addr) {
        assert(addr < SIZE);
        T nr = addr >> PAGE_ADDR_BITS;
        return arr.at(nr) != nullptr;
    }
    /**
     * get the size of the array
     *
     * @return the size
     */
    uint64_t size() { return SIZE; }

protected:
    std::array<page_type*, SIZE / (1 << PAGE_ADDR_BITS) + 1> arr;
};
#endif /* _SPARSE_ARRAY_H_ */
