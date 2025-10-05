#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <chrono>
#include <utility> // std::forward
#include <iostream>
#include <cassert>
#include "BmkAllocator.h"

namespace bmk {

    struct SmallObjBench {
        int a{};
        int b{};
    };

    struct BenchmarkResults
    {
        std::size_t Operations;
        std::chrono::milliseconds Milliseconds;
        double OpsPerSec;        // ops / second
        double MsPerOp;          // ms / operation
    };

	class Benchmark {
    
    public:

        explicit Benchmark(std::size_t i_nOperations)
            : m_numOfOperations(i_nOperations) {
        }

        // trends for Small Objects (only allocation and deallocation)
        template <typename AllocBackend>
        void BenchBulk(BmkAllocator<AllocBackend>& allocator, std::size_t size);

        template <typename AllocBackend>
        void BenchSameOrder(BmkAllocator<AllocBackend>& allocator, std::size_t size);

        template <typename AllocBackend>
        void BenchReverseOrder(BmkAllocator<AllocBackend>&, std::size_t size);

        template <typename AllocBackend>
        void BenchButterfly(BmkAllocator<AllocBackend>&, std::size_t size);

        // trends for Small Objects with new and delete
        template <typename AllocBackend, typename T, typename... Args>
        void BenchSameOrderNewDelete(BmkAllocator<AllocBackend>&, Args&&... args);

    private:

        std::size_t m_numOfOperations;

        inline static BenchmarkResults BuildResults(std::size_t ops, std::chrono::milliseconds ms)
        {
            BenchmarkResults r{};

            r.Operations = ops;
            r.Milliseconds = ms;
            double msd = static_cast<double>(ms.count());
            if (msd <= 0.0) {
                r.OpsPerSec = 0.0;
                r.MsPerOp = 0.0;
            }
            else {
                r.OpsPerSec = (static_cast<double>(ops) * 1000.0) / msd;
                r.MsPerOp = msd / static_cast<double>(ops);
            }

            return r;
        }

        inline static void PrintResults(const char* title, const BenchmarkResults& r)
        {
            std::cout << title << '\n';
            std::cout << "\tOperations:   " << r.Operations << '\n';
            std::cout << "\tElapsed ms:   " << r.Milliseconds.count() << '\n';
            std::cout << "\tOps/sec:      " << r.OpsPerSec << '\n';
            std::cout << "\tMs/op:        " << r.MsPerOp << '\n\n';
        }
	};

    /// -----------------------------------------------------------------------------
    /// Helper function to measure execution time
    /// -----------------------------------------------------------------------------
    
    template <typename F>
    std::chrono::milliseconds time_ms(F&& f) {
        using namespace std::chrono;
        auto start = high_resolution_clock::now();
        std::forward<F>(f)();
        auto end = high_resolution_clock::now();
        return duration_cast<milliseconds>(end - start);
    }

    /// -----------------------------------------------------------------------------
    /// Benchmark::BenchSameOrderNewDelete
    /// -----------------------------------------------------------------------------
    
    template <typename AllocBackend, typename T, typename... Args>
    void Benchmark::BenchSameOrderNewDelete(BmkAllocator<AllocBackend>& allocator, Args&&... args) {
        
        using namespace std::chrono;

        std::vector<T*> ptrs;
        ptrs.reserve(m_numOfOperations);

        // mesure NEW
        auto msNew = time_ms([&]() {
            for (std::size_t i = 0; i < m_numOfOperations; ++i) {
                T* obj = allocator.New<T>(std::forward<Args>(args)...);
                ptrs.push_back(obj);
            }
            });

        // mesure DELETE
        auto msDel = time_ms([&]() {
            for (std::size_t i = 0; i < m_numOfOperations; ++i) {
                allocator.Delete(ptrs[i]);
            }
            });

        // separe results
        auto rNew = BuildResults(m_numOfOperations, msNew);
        auto rDel = BuildResults(m_numOfOperations, msDel);

        std::cout << "\tNew ms: " << rNew.Milliseconds << "  Delete ms: " << rDel.Milliseconds << '\n';

        // total results
        auto msTotal = msNew + msDel;
        auto rTotal = BuildResults(m_numOfOperations, msTotal);

        //PrintResults("New (SameOrder)", rNew);
        //PrintResults("Delete (SameOrder)", rDel);
        PrintResults("Total (SameOrder)", rTotal);
    }

    /// -----------------------------------------------------------------------------
    /// Benchmark::BenchBulk
    /// -----------------------------------------------------------------------------
    
    template <typename AllocBackend>
    void Benchmark::BenchBulk(BmkAllocator<AllocBackend>& allocator, std::size_t size) {
    
        std::cout << "\n=== BenchBulk size=" << size << " ===\n";

        std::vector<std::pair<void*, std::size_t>> ptrs;
        ptrs.reserve(m_numOfOperations);

        // allocate with measurement
        auto alloc_ms = time_ms([&] {
            for (std::size_t i = 0; i < m_numOfOperations; ++i) {
                ptrs.emplace_back(allocator.Allocate(size), size); // construct directly inside the vector
            }
            });

        // free outside timing
        for (auto& pr : ptrs) allocator.Free(pr.first, pr.second);
        ptrs.clear();

        BenchmarkResults r = BuildResults(m_numOfOperations, alloc_ms);
        PrintResults("BenchBulk results (alloc only):", r);
    }

    /// -----------------------------------------------------------------------------
    /// Benchmark::BenchSameOrder
    /// -----------------------------------------------------------------------------
    
    template <typename AllocBackend>
    void Benchmark::BenchSameOrder(BmkAllocator<AllocBackend>& allocator, std::size_t size) {

        std::cout << "\n=== BenchSameOrder size=" << size << " ===\n";

        std::vector<std::pair<void*, std::size_t>> ptrs; 
        ptrs.reserve(m_numOfOperations);

        auto alloc_ms = time_ms([&] {
            for (std::size_t i = 0; i < m_numOfOperations; ++i) ptrs.emplace_back(allocator.Allocate(size), size);
            });

        auto free_ms = time_ms([&] {
            for (auto& pr : ptrs) allocator.Free(pr.first, pr.second);
            });

        BenchmarkResults r = BuildResults(m_numOfOperations, std::chrono::duration_cast<std::chrono::milliseconds>(alloc_ms + free_ms));
        std::cout << "\talloc ms: " << alloc_ms.count() << "  free ms: " << free_ms.count() << '\n';
        PrintResults("BenchSameOrder results:", r);
    }

    /// -----------------------------------------------------------------------------
    /// Benchmark::BenchReverseOrder
    /// -----------------------------------------------------------------------------

    template <typename AllocBackend>
    void Benchmark::BenchReverseOrder(BmkAllocator<AllocBackend>& allocator, std::size_t size) {

        std::cout << "\n=== BenchReverseOrder size=" << size << " ===\n";

        std::vector<std::pair<void*, std::size_t>> ptrs; 
        ptrs.reserve(m_numOfOperations);

        auto alloc_ms = time_ms([&] {
            for (std::size_t i = 0; i < m_numOfOperations; ++i) ptrs.emplace_back(allocator.Allocate(size), size);
            });

        auto free_ms = time_ms([&] {
            for (auto it = ptrs.rbegin(); it != ptrs.rend(); ++it) allocator.Free(it->first, it->second);
            });

        BenchmarkResults r = BuildResults(m_numOfOperations, std::chrono::duration_cast<std::chrono::milliseconds>(alloc_ms + free_ms));
        std::cout << "\talloc ms: " << alloc_ms.count() << "  free ms: " << free_ms.count() << '\n';
        PrintResults("BenchReverseOrder results:", r);
    }

    /// -----------------------------------------------------------------------------
    /// Benchmark::BenchButterfly
    /// -----------------------------------------------------------------------------

    template <typename AllocBackend>
    void Benchmark::BenchButterfly(BmkAllocator<AllocBackend>& allocator, std::size_t size) {

        std::cout << "\n=== BenchButterfly size=" << size << " ===\n";

        std::vector<std::pair<void*, std::size_t>> ptrs; 
        ptrs.reserve(m_numOfOperations);

        auto alloc_ms = time_ms([&] {
            for (std::size_t i = 0; i < m_numOfOperations; ++i) ptrs.emplace_back(allocator.Allocate(size), size);
            });

        auto free_ms = time_ms([&] {
            std::size_t i = 0;
            std::size_t j = ptrs.empty() ? 0 : ptrs.size() - 1;
            while (i < j) {
                allocator.Free(ptrs[i].first, ptrs[i].second);
                allocator.Free(ptrs[j].first, ptrs[j].second);
                ++i; --j;
            }
            if (i == j && !ptrs.empty()) allocator.Free(ptrs[i].first, ptrs[i].second);
            });

        BenchmarkResults r = BuildResults(m_numOfOperations, std::chrono::duration_cast<std::chrono::milliseconds>(alloc_ms + free_ms));
        std::cout << "\talloc ms: " << alloc_ms.count() << "  free ms: " << free_ms.count() << '\n';
        PrintResults("BenchButterfly results:", r);
    }
}

#endif !BENCHMARK_H