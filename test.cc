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
#include <bits/stdc++.h>
#include "tscns.h"

using namespace std;

bool cpupin(int cpuid) {
  cpu_set_t my_set;
  CPU_ZERO(&my_set);
  CPU_SET(cpuid, &my_set);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &my_set)) {
    std::cerr << "sched_setaffinity error: " << strerror(errno) << std::endl;
    return false;
  }

  return true;
}

TSCNS tn;

int main(int argc, char** argv) {
  double tsc_ghz = 0.0;
  if (argc > 1) {
    tsc_ghz = stod(argv[1]);
  }
  if (argc > 2) {
    int cpu = stoi(argv[2]);
    if (!cpupin(cpu)) exit(1);
    cout << "bind to cpu: " << cpu << " success" << endl;
  }
  tn.init(tsc_ghz);
  if (tsc_ghz <= 0.0) {
    int wait_sec = 10; // set a longer time(e.g. more than 1 min) to get more precise tsc_ghz
    cout << "waiting " << wait_sec << " secs for calibration" << endl;
    std::this_thread::sleep_for(std::chrono::seconds(wait_sec));
    tsc_ghz = tn.calibrate();
  }
  cout << std::setprecision(17) << "tsc_ghz: " << tsc_ghz << endl;

  // 1) Try binding to different cores with the same tsc_ghz at nearly the same time, see if the offsets are similar(not
  // necessary the same). If not, you're doomed: tsc of your machine's different cores are not synced up, don't share
  // TSCNS among threads then.
  //
  // 2) Try running test with the same tsc_ghz at different times, see if the offsets are similar(not necessary the
  // same). If you find them steadily go up/down at a fast speed, then your tsc_ghz is not precise enough, try
  // calibrating with a longer waiting time and test again.
  cout << "offset: " << tn.rdoffset() << endl;

  uint64_t rdns_latency;
  {
    const int N = 10;
    uint64_t nses[N + 1];
    for (int i = 0; i <= N; i++) {
      nses[i] = tn.rdns();
    }
    rdns_latency = (nses[N] - nses[0]) / N;
  }
  cout << "rdns_latency: " << rdns_latency << endl;

  while (true) {
    uint64_t a = tn.rdns();
    uint64_t b = tn.rdsysns();
    uint64_t c = tn.rdns();
    int64_t a2b = b - a;
    int64_t b2c = c - b;
    bool valid = a2b >= 0 && b2c >= 0;
    uint64_t rdsysns_latency = c - a - rdns_latency;
    cout << "a: " << a << ", b: " << b << ", c: " << c << ", a2b: " << a2b << ", b2c: " << b2c << ", valid: " << valid
         << ", rdsysns_latency: " << rdsysns_latency << endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}
