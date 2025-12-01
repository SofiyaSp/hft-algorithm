#include "algo.hpp"
#include "orderbook.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include <chrono>
#include <iomanip>

// deterministic feed generator for clean demo

Snapshot make_fake(int t) {
    Snapshot s;
    s.tstamp = t;

    // Triangle wave: base oscillates between 99.5 and 100.5 over 40 ticks
    int period = 40; // total ticks for up+down
    int phase = t % period;
    double base;
    if (phase < period / 2) {
        // rising part
        base = 99.5 + (phase / double(period/2)) * 1.0; // 99.5 -> 100.5
    } else {
        // falling part
        base = 100.5 - ((phase - period/2) / double(period/2)) * 1.0; // 100.5 -> 99.5
    }

    // simple deterministic imbalance oscillation
    double bid_qty = 10 + 5 * ((phase < period/2) ? 1 : -1); // high bid on rising, low bid on falling
    double ask_qty = 10 + 5 * ((phase < period/2) ? -1 : 1); // opposite

    double spread = 0.05;
    s.bids.push_back({base - spread, bid_qty});
    s.asks.push_back({base + spread, ask_qty});

    return s;
}

///////////////////////Noise Triangle///////////////////////////////
//simple deterministic pseudo-random in [0,1]
static double rand01(int key) {
    unsigned int x = static_cast<unsigned int>(key) * 1103515245u + 12345u;
    return (x & 0x7FFFu) / 32767.0;
}

Snapshot make_fake_noisy(int t) {
    Snapshot s;
    s.tstamp = t;

    int period = 40;
    int phase  = t % period;
    double half = period / 2.0;

    //Slow upward drift of center price
    double drift = 0.0005 * t;  // ~+0.2 over 400 ticks

    //Per-cycle random amplitude around 0.5
    int cycle = t / period;
    double amp_noise = (rand01(cycle + 1000) - 0.5) * 0.2; // [-0.1, +0.1]
    double amp = 0.5 + amp_noise;                          // ~[0.4, 0.6]

    //Normalized triangle in [-1, 1] for each cycle
    double u;
    if (phase < half) {
        u = phase / half;               //rising 0..1
    } else {
        u = 2.0 - (phase / half);       //falling 1..0
    }
    double tri_norm = 2.0 * u - 1.0;    //-1..1

    double base = (100.0 + drift) + amp * tri_norm;

    // Per-tick local noise ~[-0.02, +0.02]
    double tick_noise = (rand01(t + 5000) - 0.5) * 0.04;
    base += tick_noise;

    // Order book: imbalance still biased by leg
    double bid_qty = 10 + 5 * ((phase < half) ? 1 : -1);
    double ask_qty = 10 + 5 * ((phase < half) ? -1 : 1);

    double spread = 0.05;
    s.bids.push_back({base - spread, bid_qty});
    s.asks.push_back({base + spread, ask_qty});

    return s;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//struct for logging trade results
struct TradeRow {
    int    tstamp;
    std::string side;   //"BUY", "SELL", "NONE"
    double price;
    double pnl;
};

//run one test on the triangle wave feed
struct TestResult {
    std::string name;
    bool        pass;
    double      final_pnl;
    int         num_trades;   //BUY or SELL
    long long   micros;       //time taken in microseconds
    std::vector<TradeRow> rows;
};

using FeedGen = Snapshot(*)(int);

TestResult run_profit_test(const std::string &name, FeedGen gen, int T) {
    TestResult res;
    res.name = name;

    HFTAlgo algo(0.7, 0.1, 0.1);

    std::vector<Snapshot> feed;
    for (int t = 1; t <= T; ++t) {
        feed.push_back(gen(t));
    }

    auto start = std::chrono::high_resolution_clock::now();

    double final_pnl = 0.0;
    int num_trades = 0;

    for (auto &snap : feed) {
        TradeResult tr = algo.process_snapshot(snap);

        if (tr.side == "BUY" || tr.side == "SELL")
            num_trades++;

        res.rows.push_back({static_cast<int>(tr.tstamp), tr.side, tr.price, tr.pnl});
        final_pnl = tr.pnl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    res.micros     = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    res.final_pnl  = final_pnl;
    res.num_trades = num_trades;
    res.pass       = (num_trades > 0) && (final_pnl > 0.0);

    return res;
}

TestResult run_triangle_profit_test() {
    TestResult res;
    res.name = "TriangleWave_Test";

    HFTAlgo algo(0.7, 0.1, 0.1);  //fast alpha, slow alpha, imbalance threshold

    std::vector<Snapshot> feed;
    for (int t = 1; t <= 120; ++t) {
        feed.push_back(make_fake(t));
    }

    auto start = std::chrono::high_resolution_clock::now();

    double final_pnl = 0.0;
    int num_trades = 0;

    for (auto &snap : feed) {
        TradeResult tr = algo.process_snapshot(snap);

        // count only real entries/exits as trades
        if (tr.side == "BUY" || tr.side == "SELL") {
            num_trades++;
        }

        // log every tick (including NONE)
        TradeRow row;
        row.tstamp = tr.tstamp;
        row.side   = tr.side;
        row.price  = tr.price;
        row.pnl    = tr.pnl;
        res.rows.push_back(row);

        final_pnl = tr.pnl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    res.micros = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    res.final_pnl = final_pnl;
    res.num_trades = num_trades;

    // simple PASS/FAIL rule for this toy test:
    //  - must actually trade
    //  - must end with positive PnL
    res.pass = (num_trades > 0) && (final_pnl > 0.0);

    return res;
}

// ----------------------------
// main: run tests and print organized output

int main() {
    std::vector<TestResult> tests;

    tests.push_back(run_profit_test("TriangleWave_Test",        make_fake,       120));
    tests.push_back(run_profit_test("NoisyTriangleWave_Test",   make_fake_noisy, 120));

    for (const auto &tri : tests) {
        std::cout << "==============================\n";
        std::cout << "Test Name   : " << tri.name << "\n";
        std::cout << "Result      : " << (tri.pass ? "PASS" : "FAIL") << "\n";
        std::cout << "Final PnL   : " << tri.final_pnl << "\n";
        std::cout << "Num trades  : " << tri.num_trades << "\n";
        std::cout << "Time taken  : " << tri.micros << " us\n";
        std::cout << "==============================\n\n";

        std::cout << std::left
                  << std::setw(8)  << "T"
                  << std::setw(8)  << "SIDE"
                  << std::setw(12) << "PRICE"
                  << std::setw(12) << "PNL"
                  << "\n";
        std::cout << "--------------------------------------\n";

        std::cout << std::fixed << std::setprecision(4);
        for (const auto &r : tri.rows) {
            std::cout << std::left
                      << std::setw(8)  << r.tstamp
                      << std::setw(8)  << r.side
                      << std::setw(12) << r.price
                      << std::setw(12) << r.pnl
                      << "\n";
        }
        std::cout << "\n\n";
    }

    // overall exit code: 0 only if all tests pass
    bool all_pass = true;
    for (auto &tr : tests) if (!tr.pass) all_pass = false;
    return all_pass ? 0 : 1;
}

