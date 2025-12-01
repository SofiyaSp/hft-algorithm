#include "algo.hpp"

HFTAlgo::HFTAlgo(double a_fast, double a_slow, double thr)
{
    alpha_fast_fx = to_fixed(a_fast);
    alpha_slow_fx = to_fixed(a_slow);
    thr_fx = to_fixed(thr);
    one_fx = to_fixed(1.0);

    fastEMA = slowEMA = 0;
    has_init = false;

    cash = 100000;
    pos = 0;
}

TradeResult HFTAlgo::process_snapshot(const Snapshot &s)
{
    double mid = compute_mid(s);
    double imb = compute_imbalance(s);

    q16_16 mid_fx = to_fixed(mid);

    if (!has_init) {
        fastEMA = slowEMA = mid_fx;
        has_init = true;
    } else {
        fastEMA = qadd(qmul(alpha_fast_fx, mid_fx),
                       qmul(qsub(one_fx, alpha_fast_fx), fastEMA));

        slowEMA = qadd(qmul(alpha_slow_fx, mid_fx),
                       qmul(qsub(one_fx, alpha_slow_fx), slowEMA));
    }

  q16_16 ema_diff = qsub(fastEMA, slowEMA);
  double ema_diff_f = to_float(ema_diff);
  double thr = to_float(thr_fx);

  bool buy  = (imb >  thr && ema_diff_f >  0.01);
  bool sell = (imb < -thr && ema_diff_f < -0.01);

    TradeResult tr;
    tr.tstamp = s.tstamp;

    if (buy && pos <= 0) {
        double px = s.asks[0].price;
        cash -= px;
        pos += 1;
        tr.side = "BUY";
        tr.price = px;
        tr.pnl = cash + pos * mid - 100000;
    }
    else if (sell && pos >= 0) {
        double px = s.bids[0].price;
        cash += px;
        pos -= 1;
        tr.side = "SELL";
        tr.price = px;
        tr.pnl = cash + pos * mid - 100000;
    } else {
        tr.side = "NONE";
        tr.price = 0;
        tr.pnl = cash + pos * mid - 100000;
    }

    return tr;
}
