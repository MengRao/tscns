/*
MIT License

Copyright (c) 2022 Meng Rao <raomeng1@gmail.com>

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
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include "tscns.h"

using namespace std;

TSCNS tn;

int main(int argc, char** argv) {
  tn.init();
  cout << std::setprecision(15) << "init tsc_ghz: " << tn.getTscGhz() << endl;

  double rdns_latency;
  {
    const int N = 1000;
    int64_t tmp = 0;
    int64_t t0 = tn.rdsysns();
    for (int i = 0; i < N; i++) {
      tmp += tn.rdsysns();
    }
    int64_t t1 = tn.rdsysns();
    for (int i = 0; i < N; i++) {
      tmp += tn.rdtsc();
    }
    int64_t t2 = tn.rdsysns();
    for (int i = 0; i < N; i++) {
      tmp += tn.rdns();
    }
    int64_t t3 = tn.rdsysns();
    // rdsys_latency is actually a low bound here as it's measured in a busy loop
    double rdsys_latency = (double)(t1 - t0) / (N + 1);
    double rdtsc_latency = (double)(t2 - t1 - rdsys_latency) / N;
    rdns_latency = (double)(t3 - t2 - rdsys_latency) / N;
    cout << "rdsys_latency: " << rdsys_latency << ", rdtsc_latency: " << rdtsc_latency
         << ", rdns_latency: " << rdns_latency << ", tmp: " << tmp << endl;
  }

  while (true) {
    int64_t a = tn.rdns();
    tn.calibrate();
    int64_t b = tn.rdns();
    int64_t c = tn.rdsysns();
    int64_t d = tn.rdns();
    int64_t b2c = c - b;
    int64_t c2d = d - c;
    int64_t err = 0;
    if (b2c < 0)
      err = -b2c;
    else if (c2d < 0)
      err = c2d;
    // calibrate_latency should not be a large value, especially not negative
    int64_t calibrate_latency = b - a - (int64_t)rdns_latency;
    int64_t rdsysns_latency = d - b - (int64_t)rdns_latency;
    cout << "calibrate_latency: " << calibrate_latency << ", tsc_ghz: " << tn.getTscGhz()
         << ", b2c: " << b2c << ", c2d: " << c2d << ", err: " << err
         << ", rdsysns_latency: " << rdsysns_latency << endl;
    auto expire = tn.rdns() + tn.NsPerSec / 2;
    while (tn.rdns() < expire) std::this_thread::yield();
  }

  return 0;
}
