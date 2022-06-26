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
TSCNS tn;

tn.init();
```

Getting nanosecond timestamp in a single step:
```C++
int64_t ns = tn.rdns();
```

Or just recording a tsc in some time-critical tasks and converting it to ns in jobs that can be delayed:
```C++
// in time-critical task
int64_t tsc = tn.rdtsc();
...
// in logging task
int64_t ns = tn.tsc2ns(tsc);
```

Calibration with some interval in the background:
```C++
while(running) {
  tn.calibrate();
  std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

## More about calibration
todo
