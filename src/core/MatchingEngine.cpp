#include "MatchingEngine.hpp"


std::vector<Trade> MatchingEngine::processOrder(Order order) {
    return book_.processOrder(std::move(order));
}


OrderBook& MatchingEngine::getOrderBook() const {
    return book_;
}
