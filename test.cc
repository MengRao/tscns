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
    if (!cpupin(cpu, 0)) exit(1);
    cout << "bind to cpu: " << cpu << " success" << endl;
  }
  tn.init(tsc_ghz);
  if (tsc_ghz <= 0.0) {
    int wait_sec = 10;
    cout << "waiting " << wait_sec << " secs for calibration" << endl;
    std::this_thread::sleep_for(std::chrono::seconds(wait_sec));
    double tsc_ghz = tn.calibrate();
    cout << std::setprecision(15) << "tsc_ghz: " << tsc_ghz << endl;
  }

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