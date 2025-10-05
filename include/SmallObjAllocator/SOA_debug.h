#ifndef SOA_DEBUG_H
#define SOA_DEBUG_H

#include <deque>
#include <map>
#include <cstdint>

//#define SOA_DEBUG_LOG_ENABLED

#ifdef SOA_DEBUG_LOG_ENABLED

#include <iostream>
#include <sstream>
#include "Chunk.h"

/// -----------------------------------------------------------------------------
/// Base logging macros/lambdas
/// -----------------------------------------------------------------------------

    constexpr auto SOA_LOG = [](const char* msg) {
        std::cout << msg << std::endl;
        };

#define SOA_LOG_OSS(x) do { \
    std::ostringstream oss_; \
    oss_ << x; \
    std::cout << oss_.str() << std::endl; \
} while(0)

/// -----------------------------------------------------------------------------
/// Debug helpers for allocator internals
/// -----------------------------------------------------------------------------

    inline void SOA_PrintChunks(const std::deque<soa::Chunk>&chunks) {
        std::ostringstream oss;
        oss << "[DEBUG] m_chunks: size=" << chunks.size();
        std::cout << oss.str() << std::endl;

        for (std::size_t i = 0; i < chunks.size(); ++i) {
            const soa::Chunk& c = chunks[i];
            std::ostringstream chunkInfo;
            chunkInfo << "  Chunk[" << i << "] @ " << &c
                << "  freeBlocks=" << static_cast<int>(c.m_blocksAvailable)
                << "  m_pData=" << static_cast<void*>(c.m_pData);
            std::cout << chunkInfo.str() << std::endl;
        }
    }

    inline void SOA_PrintChunkMap(const std::map<std::uintptr_t, soa::Chunk*>&m) {
        std::ostringstream oss;
        oss << "[DEBUG] m_chunkMap: size=" << m.size();
        std::cout << oss.str() << std::endl;

        for (const auto& [addr, ptr] : m) {
            std::ostringstream entry;
            entry << "  key=0x" << std::hex << addr << std::dec
                << " -> Chunk*=" << ptr
                << " (m_pData=" << static_cast<void*>(ptr ? ptr->m_pData : nullptr) << ")";
            std::cout << entry.str() << std::endl;
        }
    }


#else
    constexpr auto SOA_LOG = []([[maybe_unused]] const char* msg) noexcept { /* no op */ };

#define SOA_LOG_OSS(x) do {} while(0)

    inline void SOA_PrintChunks([[maybe_unused]] const std::deque<soa::Chunk>&) {}
    inline void SOA_PrintChunkMap([[maybe_unused]] const std::map<std::uintptr_t, soa::Chunk*>&) {}

#endif // !SOA_DEBUG_LOG_ENABLED

#endif // !SOA_DEBUG_H