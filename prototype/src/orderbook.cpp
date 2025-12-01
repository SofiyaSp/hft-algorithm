// THIS MODULE IS TO MIMIC THE ORDERBOOK FUNCTIONALITY FOR THE PURPOSE OF TESTING THE ALGORITHM

#include "orderbook.hpp"

double compute_mid(const Snapshot &s) {
    return 0.5 * (s.bids[0].price + s.asks[0].price);
}

double compute_imbalance(const Snapshot &s) {
    double b = 0, a = 0;
    for (auto &x : s.bids) b += x.qty;
    for (auto &x : s.asks) a += x.qty;
    return (b - a) / (b + a + 1e-9);
}
