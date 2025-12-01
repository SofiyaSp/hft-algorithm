#pragma once
#include <vector>

struct Level {
    double price;
    double qty;
};

struct Snapshot {
    long long tstamp;
    std::vector<Level> bids;
    std::vector<Level> asks;
};

double compute_mid(const Snapshot &s);
double compute_imbalance(const Snapshot &s);
