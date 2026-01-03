#include "FxTradeLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <stdexcept>

FxTrade* FxTradeLoader::createTradeFromLine(const std::string& line) {
    std::vector<std::string> items;
    std::stringstream ss(line);
    std::string item;

    std::size_t start = 0;
    for (;;) {
        auto pos = line.find(separator, start);
        if (std::string::npos == pos) {
            items.push_back(line.substr(start));
            break;
        }
        items.push_back(line.substr(start, pos - start));
        start = pos + sizeof(separator) / sizeof(separator[0]) - 1;
    }

    // std::cout << "Parsed " << items.size() << " items from line: " << line << std::endl;
    // for (size_t i = 0; i < items.size(); ++i) {
    //     std::cout << "  Item " << i << ": " << items[i] << std::endl;
    // }

    if (items.size() != 9) {
        throw std::runtime_error("Invalid line format");
    }

    const auto tradeType = items[0] == "FxSpot" ? FxTrade::FxSpotTradeType : FxTrade::FxForwardTradeType;
    FxTrade* trade = new FxTrade(items[8], tradeType);

    auto get_tp = [](const std::string& ymd) {
        std::tm tm = {};
        std::istringstream dateStream(ymd);
        dateStream >> std::get_time(&tm, "%Y-%m-%d");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    };

    trade->setTradeDate(get_tp(items[1]));
    trade->setValueDate(get_tp(items[6]));

    // The Instrument field should be constructed as the concatenation of Ccy1 and Ccy2
    trade->setInstrument(items[2] + items[3]);
    trade->setCounterparty(items[7]);
    trade->setNotional(std::stod(items[4]));
    trade->setRate(std::stod(items[5]));

    return trade;
}

std::vector<ITrade*> FxTradeLoader::loadTrades() {
    if (dataFile_.empty()) {
        throw std::invalid_argument("Filename cannot be null");
    }

    std::string line;
    std::ifstream stream(dataFile_);
    if (!stream.is_open()) {
        throw std::runtime_error("Cannot open file: " + dataFile_);
    }

    std::vector<std::string> lines;

    std::getline(stream, line); // skip header
    std::getline(stream, line); // skip header
    while (std::getline(stream, line)) {
        // Remove CR on Linux
        if (!line.empty() and '\r' == line.back())
            line.pop_back();
        lines.push_back(line);
    }
    // Skip footer
    if (!lines.empty() and "END¬5" == lines.back())
        lines.pop_back();
    
    


    std::vector<ITrade*> result;
    for (size_t i = 0; i < lines.size(); ++i) {
        result.push_back(createTradeFromLine(lines[i]));
    }
    return result;
}

std::string FxTradeLoader::getDataFile() const {
    return dataFile_;
}

void FxTradeLoader::setDataFile(const std::string& file) {
    dataFile_ = file;
}
