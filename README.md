# TSCNS 2.0
## What's the problem with clock_gettime/gettimeofday/std::chrono::XXX_clock?
Although current Linux systems are using VDSO to implement clock_gettime/gettimeofday/std::chrono::XXX_clock, they still have a nonnegligible overhead with latency from 20 to 100 ns. The problem is even worse on Windows as the latency is more unstable and could be as high as 1 us, also on Windows, the high resolution clock is at only 100 ns precison.

These problems are not good for time-critical tasks where high precison timestamp is required and latency of getting timestamp itself should be minimized.

## How is TSCNS different?
TSCNS uses rdtsc instruction and simple arithmatic operations to implement a thread-safe clock with 1 ns precision, and is much faster and stable in terms of latency in less than 10 ns, comprising latency of rdtsc(4 ~ 7 ns depending on platforms) plus calculations in less than 1 ns.

Also it can be closely synchronized with the system clock, which makes it a good alternative of standard system clocks. However, real-time synchronization requires the clock to be calibrated at a proper interval, but it's a easy and cheap job to do.

## Usage
Initialization:
```C++
TSCNS tscns;

tscns.init();
```

Getting nanosecond timestamp in a single step:
```C++
int64_t ns = tscns.rdns();
```

Or just recording a tsc in some time-critical tasks and converting it to ns in jobs that can be delayed:
```C++
// in time-critical task
int64_t tsc = tscns.rdtsc();
...
// in logging task
int64_t ns = tscns.tsc2ns(tsc);
```

Calibration with some interval in the background:
```C++
while(running) {
  tscns.calibrate();
  std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

## More about calibration
Actually the init function has two optional parameters: `void init(int64_t init_calibrate_ns, int64_t calibrate_interval_ns)`: the initial calibration wait time and afterwards calibration interval. The initial calibration is used to find a proper tsc frequency to start with, and it's blocking in `tscns.init()`, so the default wait time is set to a small value: 20 ms. User can choose to wait a longer time for a more precise initial calibration, e.g. 1 second.

`calibrate_interval_ns` sets the minimum calibration interval to keep tscns synced with system clock, the default value is 3 seconds. Also user need to call `calibrate()` function to trigger calibration in an interval no larger than `calibrate_interval_ns`. The `calibrate()` function is non-blocking and cheap to call but not thread-safe, so user should have only one thread calling it. The calibrations will adjust tsc frequency in the library to trace that of the system clock and keep timestamp divergence in a minimum level. During calibration, `rdns()` results in other threads is guaranteed to be continuous: there won't be jump in values and especially timestamp won't go backwards. Below picture shows how these routine calibrations suppress timestamp error caused by the initial coarse calibration and system clock speed correction. Also user can choose not to calibrate after initialization: just don't call the `calibrate()` function and tscns will always go with the initial tsc frequency.

![tscns](https://user-images.githubusercontent.com/11496526/175851336-b92dc8f2-ef6b-4c03-80ec-b7c4e36b2784.png)

## Differences with TSCNS 1.0
* TSCNS 2.0 supports routine calibrations in addition to only initial calibration in 1.0, so time drifting awaying from system clock can be radically eliminated. Also tsc_ghz can't be set by the user any more and the cheat method in 1.0 are also obsolete. In 2.0, `tsc2ns()` added a sequence lock to protect from parameters change caused by calibrations, the added performance cost is less than 0.5 ns.
* Windows is supported now. We believe Windows applications will benefit much more from TSCNS because of the drawbacks of the system clock we mentioned at the beginning.
