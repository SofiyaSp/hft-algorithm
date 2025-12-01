#pragma once
#include "fixedPoint.hpp"
#include "orderbook.hpp"
#include <string>


struct TradeResult {
    long long tstamp;
    std::string side;
    double price;
    double pnl;
};

class HFTAlgo {
public:
    HFTAlgo(double alpha_fast, double alpha_slow, double thr);

    TradeResult process_snapshot(const Snapshot &s);

private:
    q16_16 fastEMA, slowEMA;
    bool has_init;

    double cash;
    int pos;
    double last_mid;

    q16_16 alpha_fast_fx, alpha_slow_fx, one_fx, thr_fx;
};
