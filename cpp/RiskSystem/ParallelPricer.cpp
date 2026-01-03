#include "ParallelPricer.h"
#include <stdexcept>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <algorithm>
#include "../Pricers/GovBondPricingEngine.h"
#include "../Pricers/CorpBondPricingEngine.h"
#include "../Pricers/FxPricingEngine.h"

ParallelPricer::~ParallelPricer() {

}

void ParallelPricer::loadPricers() {
    PricingConfigLoader pricingConfigLoader;
    pricingConfigLoader.setConfigFile("./PricingConfig/PricingEngines.xml");
    PricingEngineConfig pricerConfig = pricingConfigLoader.loadConfig();
    
    using Factory = std::function<IPricingEngine*()>;
    const std::unordered_map<std::string, Factory> factories = {
        { "HmxLabs.TechTest.Pricers.GovBondPricingEngine", [] { return new GovBondPricingEngine; } },
        { "HmxLabs.TechTest.Pricers.CorpBondPricingEngine", [] { return new CorpBondPricingEngine; } },
        { "HmxLabs.TechTest.Pricers.FxPricingEngine", [] { return new FxPricingEngine; } },
    };
    
    for (const auto& configItem : pricerConfig) {
        IPricingEngine* pricer = nullptr;
        auto name = configItem.getTypeName();
        if (factories.find(name) != factories.end())
            pricers_[configItem.getTradeType()]  = factories.at(name)();
        else
            throw std::runtime_error("Unknown pricing engine type: " + name);
    }
}

void ParallelPricer::price(const std::vector<std::vector<ITrade*>>& tradeContainers, 
                           IScalarResultReceiver* resultReceiver) {

    loadPricers();

    if (!resultReceiver) {
        throw std::invalid_argument("Result receiver cannot be null");
    }

    std::vector<ITrade*> allTrades;
    for (const auto& container : tradeContainers) {
        allTrades.insert(allTrades.end(), container.begin(), container.end());
    }

    class SynchronisedResultReceiver : public IScalarResultReceiver {
    public:
        SynchronisedResultReceiver(IScalarResultReceiver* inner, std::mutex* mutex)
            : inner_(inner),
              mutex_(mutex) {}

        void addResult(const std::string& tradeId, double result) override {
            // The underlying receiver (ScalarResults) is not thread-safe.
            std::scoped_lock<std::mutex> lock {*mutex_};
            inner_->addResult(tradeId, result);
        }

        void addError(const std::string& tradeId, const std::string& error) override {
            // The underlying receiver (ScalarResults) is not thread-safe.
            std::scoped_lock<std::mutex> lock {*mutex_};
            inner_->addError(tradeId, error);
        }

    private:
        IScalarResultReceiver* inner_ = nullptr;
        std::mutex* mutex_ = nullptr;
    };

    SynchronisedResultReceiver syncReceiver(resultReceiver, &resultMutex_);

    std::atomic<size_t> nextIndex {0};
    const size_t totalTrades = allTrades.size();
    size_t workerCount = std::max(1u, std::thread::hardware_concurrency());
    if (workerCount > totalTrades && totalTrades > 0) {
        workerCount = totalTrades;
    }

    std::vector<std::jthread> workers;
    workers.reserve(workerCount);
    for (size_t i = 0; i < workerCount; ++i) {
        workers.emplace_back([this, &allTrades, totalTrades, &nextIndex, &syncReceiver]() {
            for (;;) {
                const size_t index = nextIndex.fetch_add(1);
                if (index >= totalTrades)
                    return;

                auto* trade = allTrades[index];
                const auto tradeType = trade->getTradeType();
                auto it = pricers_.find(tradeType);
                if (it == pricers_.end()) {
                    syncReceiver.addError(trade->getTradeId(),
                        "No Pricing Engines available for this trade type");
                    continue;
                }
                it->second->price(trade, &syncReceiver);
            }
        });
    }
}
