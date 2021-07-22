/*
MIT License

Copyright (c) 2019 Meng Rao <raomeng1@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include <time.h>

class TSCNS
{
public:
  // If you haven't calibrated tsc_ghz on this machine, set tsc_ghz as 0.0 and it will auto wait 10 ms and calibrate.
  // Of course you can calibrate again later(e.g. after system init is done) and the longer you wait the more precise
  // tsc_ghz calibrate can get. It's a good idea that user waits as long as possible(more than 1 min) once, and save the
  // resultant tsc_ghz returned from calibrate() somewhere(e.g. config file) on this machine for future use. Or you can
  // cheat, see README and cheat.cc for details.
  //
  // If you have calibrated/cheated before on this machine as above, set tsc_ghz and skip calibration.
  //
  // One more thing: you can re-init and calibrate TSCNS at later times if you want to re-sync with
  // system time in case of NTP or manual time changes.
  double init(double tsc_ghz = 0.0) {
    syncTime(base_tsc, base_ns);
    if (tsc_ghz > 0) {
      tsc_ghz_inv = 1.0 / tsc_ghz;
      adjustOffset();
      return tsc_ghz;
    }
    else {
      return calibrate();
    }
  }

  double calibrate(int64_t min_wait_ns = 10000000) {
    int64_t delayed_tsc, delayed_ns;
    do {
      syncTime(delayed_tsc, delayed_ns);
    } while ((delayed_ns - base_ns) < min_wait_ns);
    tsc_ghz_inv = (double)(delayed_ns - base_ns) / (delayed_tsc - base_tsc);
    adjustOffset();
    return 1.0 / tsc_ghz_inv;
  }

  static int64_t rdtsc() { return __builtin_ia32_rdtsc(); }

  int64_t tsc2ns(int64_t tsc) const { return ns_offset + (int64_t)(tsc * tsc_ghz_inv); }

  int64_t rdns() const { return tsc2ns(rdtsc()); }

  // If you want cross-platform, use std::chrono as below which incurs one more function call:
  // return std::chrono::high_resolution_clock::now().time_since_epoch().count();
  static int64_t rdsysns() {
    timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
  }

  // For checking purposes, see test.cc
  int64_t rdoffset() const { return ns_offset; }

  // Linux kernel sync time by finding the first try with tsc diff < 50000
  // We do better: we find the try with the mininum tsc diff
  void syncTime(int64_t& tsc, int64_t& ns) {
    const int N = 10;
    int64_t tscs[N + 1];
    int64_t nses[N + 1];

    tscs[0] = rdtsc();
    for (int i = 1; i <= N; i++) {
      nses[i] = rdsysns();
      tscs[i] = rdtsc();
    }

    int best = 1;
    for (int i = 2; i <= N; i++) {
      if (tscs[i] - tscs[i - 1] < tscs[best] - tscs[best - 1]) best = i;
    }
    tsc = (tscs[best] + tscs[best - 1]) >> 1;
    ns = nses[best];
  }

  void adjustOffset() { ns_offset = base_ns - (int64_t)(base_tsc * tsc_ghz_inv); }

  alignas(64) double tsc_ghz_inv = 1.0; // make sure tsc_ghz_inv and ns_offset are on the same cache line
  int64_t ns_offset = 0;
  int64_t base_tsc = 0;
  int64_t base_ns = 0;
};
