/*
 * Copyright (C) 2020 ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <seastar/core/align.hh>
#include <cstring>

// defined by linker
extern char __executable_start;
extern char __etext;

volatile int qq;

void relocate_program_text_to_huge_pages() {
#ifdef MFD_CLOEXEC
    size_t huge_page_size = 2 << 20;
    //auto start = seastar::align_down(&__executable_start, huge_page_size);
    auto start = reinterpret_cast<char*>(0x600000);
    auto end = &__etext;
    auto aligned_start = seastar::align_down(start, huge_page_size);
    auto aligned_end = seastar::align_up(end, huge_page_size);
    auto delta = start - aligned_start;
    auto fd = memfd_create("hugepage-text-segment", MFD_CLOEXEC);
    auto size = end - start;
    auto aligned_size = aligned_end - aligned_start;
    auto padded_size = aligned_size + huge_page_size;
    ftruncate(fd, size);
    auto reservation = mmap(NULL, padded_size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
    auto addr = seastar::align_up(reinterpret_cast<char*>(reservation), huge_page_size);
    mmap(addr, aligned_size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
    madvise(addr, aligned_size, MADV_HUGEPAGE);
    std::memcpy(addr + delta, start, size);
    munmap(reservation, padded_size);
    mmap(aligned_start, aligned_size, PROT_READ | PROT_EXEC, MAP_FIXED | MAP_SHARED, fd, 0);
    madvise(aligned_start, aligned_size, MADV_HUGEPAGE);
    for (ssize_t i = 0; i < aligned_size; i += 4096) {
	qq += aligned_start[i];
    }
#endif
}
