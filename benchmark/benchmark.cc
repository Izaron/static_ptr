#include "static_ptr.h"
#include <vector>
#include <iostream>
#include <benchmark/benchmark.h>

namespace {

class IEngine {
public:
    IEngine(uint64_t& counter) : Counter_{counter} {}
    virtual ~IEngine() = default;
    virtual void Do() = 0;

protected:
    uint64_t& Counter_;
};

class TSteamEngine : public IEngine {
public:
    using IEngine::IEngine;
    void Do() override {
        ++Counter_;
    }
};

class TJetEngine : public IEngine {
public:
    using IEngine::IEngine;
    void Do() override {
        Counter_ += 5;
    }
};

class TSupersonicEngine : public IEngine {
public:
    using IEngine::IEngine;
    void Do() override {
        Counter_ += 30;
    }
};

template<typename SmartPtr>
void BM_SingleSmartPointer(benchmark::State& state) {
    constexpr bool is_unique_ptr = std::is_same_v<std::unique_ptr<IEngine>, SmartPtr>;

    uint64_t counter = 0;
    uint64_t pass = 0;
    for (auto _ : state) {
        SmartPtr ptr;

        switch (pass) {
        case 0:
            if constexpr (is_unique_ptr) {
                ptr = std::unique_ptr<IEngine>(new TSteamEngine(counter));
            } else {
                ptr.template emplace<TSteamEngine>(counter);
            }
            pass = 1;
            break;
        case 1:
            if constexpr (is_unique_ptr) {
                ptr = std::unique_ptr<IEngine>(new TJetEngine(counter));
            } else {
                ptr.template emplace<TJetEngine>(counter);
            }
            pass = 2;
            break;
        case 2:
            if constexpr (is_unique_ptr) {
                ptr = std::unique_ptr<IEngine>(new TSupersonicEngine(counter));
            } else {
                ptr.template emplace<TSupersonicEngine>(counter);
            }
            pass = 0;
            break;
        }

        ptr->Do();

        benchmark::DoNotOptimize(ptr.get());
        benchmark::ClobberMemory();
    }
    benchmark::DoNotOptimize(counter);
}

template<typename SmartPtr>
void BM_IteratingOverSmartPointer(benchmark::State& state) {
    constexpr bool is_unique_ptr = std::is_same_v<std::unique_ptr<IEngine>, SmartPtr>;

    uint64_t counter = 0;
    std::vector<SmartPtr> v;
    for (std::size_t i = 0; i < 128; ++i) {
        if (i % 3 == 0) {
            if constexpr (is_unique_ptr) {
                v.emplace_back(new TSteamEngine(counter));
            } else {
                v.emplace_back(sp::make_static<TSteamEngine>(counter));
            }
        } else if (i % 3 == 1) {
            if constexpr (is_unique_ptr) {
                v.emplace_back(new TJetEngine(counter));
            } else {
                v.emplace_back(sp::make_static<TJetEngine>(counter));
            }
        } else if (i % 3 == 2) {
            if constexpr (is_unique_ptr) {
                v.emplace_back(new TSupersonicEngine(counter));
            } else {
                v.emplace_back(sp::make_static<TSupersonicEngine>(counter));
            }
        }
    }

    for (auto _ : state) {
        for (auto& ptr : v) {
            ptr->Do();
        }
    }
    benchmark::DoNotOptimize(counter);
}

} // namespace

BENCHMARK(BM_SingleSmartPointer<std::unique_ptr<IEngine>>);
BENCHMARK(BM_SingleSmartPointer<sp::static_ptr<IEngine>>);

BENCHMARK(BM_IteratingOverSmartPointer<std::unique_ptr<IEngine>>);
BENCHMARK(BM_IteratingOverSmartPointer<sp::static_ptr<IEngine>>);

BENCHMARK_MAIN();
