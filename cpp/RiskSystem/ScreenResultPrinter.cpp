#include "ScreenResultPrinter.h"
#include <iostream>

void ScreenResultPrinter::printResults(ScalarResults& results) {
    for (const auto& result : results) {
        // Write code here to print out the results such that we have:
        // TradeID : Result : Error
        // If there is no result then the output should be:
        // TradeID : Error
        // If there is no error the output should be:
        // TradeID : Result
        auto res = result.getResult();
        auto err = result.getError();
        if (res.has_value() and err.has_value())
            std::cout << result.getTradeId() << " : " << *res << " : " << *err << std::endl;
        else if (res.has_value())
            std::cout << result.getTradeId() << " : " << *res << std::endl;
        else if (err.has_value())
            std::cout << result.getTradeId() << " : " << *err << std::endl;
    }
}
