#include "SerialPricer.h"
#include "../Pricers/GovBondPricingEngine.h"
#include "../Pricers/CorpBondPricingEngine.h"
#include "../Pricers/FxPricingEngine.h"
#include <stdexcept>
#include <unordered_map>
#include <functional>

SerialPricer::~SerialPricer() {

}

void SerialPricer::loadPricers() {
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

void SerialPricer::price(const std::vector<std::vector<ITrade*>>& tradeContainers, 
                         IScalarResultReceiver* resultReceiver) {
    loadPricers();
    
    for (const auto& tradeContainer : tradeContainers) {
        for (ITrade* trade : tradeContainer) {
            std::string tradeType = trade->getTradeType();
            if (pricers_.find(tradeType) == pricers_.end()) {
                resultReceiver->addError(trade->getTradeId(), "No Pricing Engines available for this trade type");
                continue;
            }
            
            IPricingEngine* pricer = pricers_[tradeType];
            pricer->price(trade, resultReceiver);
        }
    }
}
