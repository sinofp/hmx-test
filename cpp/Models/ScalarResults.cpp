#include "ScalarResults.h"
#include <stdexcept>

ScalarResults::~ScalarResults() = default;

std::optional<ScalarResult> ScalarResults::operator[](const std::string& tradeId) const {
    if (!containsTrade(tradeId)) {
        return std::nullopt;
    }

    std::optional<double> priceResult = std::nullopt;
    std::optional<std::string> error = std::nullopt;

    auto resultIt = results_.find(tradeId);
    if (resultIt != results_.end()) {
        priceResult = resultIt->second;
    }

    auto errorIt = errors_.find(tradeId);
    if (errorIt != errors_.end()) {
        error = errorIt->second;
    }

    return ScalarResult(tradeId, priceResult, error);
}

bool ScalarResults::containsTrade(const std::string& tradeId) const {
    return results_.find(tradeId) != results_.end() || errors_.find(tradeId) != errors_.end();
}

void ScalarResults::addResult(const std::string& tradeId, double result) {
    results_[tradeId] = result;
}

void ScalarResults::addError(const std::string& tradeId, const std::string& error) {
    errors_[tradeId] = error;
}

ScalarResults::Iterator::Iterator(const ScalarResults* parent,
                                  std::map<std::string, double>::const_iterator resultsIt,
                                  std::map<std::string, std::string>::const_iterator errorsIt,
                                  bool inErrors)
    : parent_(parent),
      resultsIt_(resultsIt),
      errorsIt_(errorsIt),
      inErrors_(inErrors) {
    if (inErrors_)
        nextError();
}

// My strategy is to output results first, then errors for trades without results
void ScalarResults::Iterator::nextError() {
    while (errorsIt_ != parent_->errors_.end() &&
           parent_->results_.find(errorsIt_->first) != parent_->results_.end()) {
        ++errorsIt_;
    }
}

ScalarResults::Iterator& ScalarResults::Iterator::operator++() {
    if (!inErrors_) {
        if (resultsIt_ != parent_->results_.end()) {
            ++resultsIt_;
        }
        if (resultsIt_ == parent_->results_.end()) {
            inErrors_ = true;
            nextError();
        }
        return *this;
    }
    
    if (errorsIt_ != parent_->errors_.end()) {
        ++errorsIt_;
        nextError();
    }
    return *this;
}

ScalarResult ScalarResults::Iterator::operator*() const {
    if (!inErrors_) {
        if (resultsIt_ == parent_->results_.end()) {
            throw std::runtime_error("Iterator dereferenced at invalid position");
        }
        auto result = (*parent_)[resultsIt_->first];
        if (!result.has_value()) {
            throw std::runtime_error("Iterator dereferenced at invalid position");
        }
        return result.value();
    }

    if (errorsIt_ == parent_->errors_.end()) {
        throw std::runtime_error("Iterator dereferenced at invalid position");
    }
    auto result = (*parent_)[errorsIt_->first];
    if (!result.has_value()) {
        throw std::runtime_error("Iterator dereferenced at invalid position");
    }
    return result.value();
}

bool ScalarResults::Iterator::operator!=(const Iterator& other) const {
    return resultsIt_ != other.resultsIt_ || errorsIt_ != other.errorsIt_;
}

ScalarResults::Iterator ScalarResults::begin() const {
    return Iterator(this, results_.begin(), errors_.begin(), results_.empty());
}

ScalarResults::Iterator ScalarResults::end() const {
    return Iterator(this, results_.end(), errors_.end(), true);
}
