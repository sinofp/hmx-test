#include "PricingConfigLoader.h"
#include <stdexcept>
#include <fstream>

std::string PricingConfigLoader::getConfigFile() const {
    return configFile_;
}

void PricingConfigLoader::setConfigFile(const std::string& file) {
    configFile_ = file;
}

PricingEngineConfig PricingConfigLoader::loadConfig() {
    PricingEngineConfig config;
    if (configFile_.empty()) {
        throw std::invalid_argument("Config file name cannot be empty");
    }
    std::ifstream stream(configFile_);
    if (!stream.is_open()) {
        throw std::runtime_error("Cannot open config file: " + configFile_);
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() and '\r' == line.back())
            line.pop_back();
        
        // I hope nobody's using such XML parsing in production code
        if (line.find("<Engine ") != std::string::npos) {
            PricingEngineConfigItem item;

            auto get = [&line](const std::string& attr) {
                auto attr_quote = attr + "=\"";
                auto start = line.find(attr_quote);
                start += attr_quote.length();
                return line.substr(start, line.find("\"", start) - start);
            };

            item.setTradeType(get("tradeType"));
            item.setAssembly(get("assembly"));
            item.setTypeName(get("pricingEngine"));

            config.push_back(item);
        }
    }
    return config;
}
