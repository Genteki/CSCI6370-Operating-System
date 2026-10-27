# PA2 Evaluation Results

Student: Kaiyuan Zhang (811597925)

Test date: October 26, 2026

Test host: `gentekis-ubuntu`, Linux x86_64, GCC 15.2.0

This host is a Linux compatibility test environment, not `odin.cs.uga.edu`. A final build and smoke test on Odin are still required by the assignment specification.

## 1. Build test

Command:

```sh
make clean all
```

Result: PASS. `webserver`, `webserver_multi`, and `client` were built successfully.

The compiler reported warnings in the supplied `net.c`, including incorrect `printf` format specifiers and possible path truncation. A separate `-Wall -Wextra -Wpedantic` build also reported unused parameters and an unused variable in `webserver_multi.c`.

## 2. Performance scaling

Each configuration served 20 simultaneous requests for `Makefile`. Each request executes the required one-second delay in `process()`. Three trials were performed for every worker count.

| Workers | Trial 1 (s) | Trial 2 (s) | Trial 3 (s) | Median (s) | HTTP 200 responses |
|---:|---:|---:|---:|---:|---:|
| 1 | 20.017 | 20.010 | 20.010 | 20.010 | 20/20 in every trial |
| 2 | 10.009 | 10.009 | 10.008 | 10.009 | 20/20 in every trial |
| 5 | 4.008 | 4.007 | 4.007 | 4.007 | 20/20 in every trial |
| 10 | 2.008 | 2.012 | 2.008 | 2.008 | 20/20 in every trial |
| 20 | 1.010 | 1.011 | 1.010 | 1.010 | 20/20 in every trial |

Relative to one worker, the median speedups were approximately 2.00x, 4.99x, 9.97x, and 19.81x for 2, 5, 10, and 20 workers, respectively. The results demonstrate that `process(fd)` executes concurrently and that increasing the worker-pool size reduces end-to-end processing time.

## 3. Full-buffer and deadlock stress test

Configuration: 10 workers, 200 simultaneous requests, 100-entry request queue, 45-second timeout.

Result: PASS.

- Completion time: 20.116 seconds
- HTTP 200 responses: 200/200
- Timeout status: completed normally
- Post-test probe: HTTP 200

Because the number of simultaneous requests exceeded the queue capacity, this test exercised the path where the listener waits on `sem_empty`. Workers continued removing requests and posting `sem_empty`, so the workload completed and the server remained responsive. No deadlock was observed.

This empirical test supports, but does not mathematically prove, deadlock freedom. The synchronization order supplies the theoretical argument: threads wait on `sem_empty` or `sem_full` before acquiring the queue mutex, and release the mutex before performing request processing. A thread therefore does not hold the queue mutex while waiting for the buffer state to change, preventing a circular wait involving the queue mutex.

## 4. Thread-count boundary tests

| Test | Result |
|---|---|
| 0 workers | Rejected with `# of threads must >= 1` |
| 101 workers | Rejected with `# of threads must <= 100` |
| 100 workers, 100 requests | 100/100 HTTP 200 responses in 2.077 seconds; post-test probe returned HTTP 200 |

## 5. ThreadSanitizer race test

The sanitizer build used:

```sh
gcc -g -O1 -pthread -fsanitize=thread -fno-omit-frame-pointer \
    -o webserver_multi_tsan webserver_multi.c net.c
```

Control test with one worker and 10 simultaneous clients:

- HTTP 200 responses: 10/10
- ThreadSanitizer race reports: 0

Concurrent test with 20 workers and 100 simultaneous clients:

- HTTP 200 responses: 96/100
- ThreadSanitizer race reports: 3
- Two reports originated from concurrent calls to `strtok()` at `net.c:133-134`.
- One report originated from concurrent calls to `gmtime()` at `net.c:51`.
- ThreadSanitizer did not report a race in the request queue, `rq.head`, or `rq.tail`.

Overall result: FAIL for a whole-program claim of race freedom. The mutex/semaphore protection of the bounded request queue passed the observed tests, but the supplied request-processing code contains non-reentrant library calls that race when multiple workers execute `process()` concurrently. The `strtok()` races also caused four empty HTTP replies during the sanitizer workload.

Before claiming that the entire program is race-free, replace `strtok()` with `strtok_r()` and `gmtime()` with `gmtime_r()`, then repeat the ThreadSanitizer test. The use of `srand()`/`rand()` and `inet_ntoa()` should also be reviewed for thread safety.

## 6. Scaling with 100 simultaneous requests

The larger workload used worker-pool sizes of 1, 2, 5, 10, 20, 50, and 100. Each configuration was measured three times, and the arithmetic average was used for the comparison plot. The ideal model is `ceil(100 / workers)` seconds because `process()` contains a required one-second delay.

| Workers | Ideal (s) | Trial 1 (s) | Trial 2 (s) | Trial 3 (s) | Average (s) | Acceleration ratio |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 100.000 | 100.021 | 100.022 | 100.022 | 100.022 | 1.00x |
| 2 | 50.000 | 50.015 | 50.015 | 50.015 | 50.015 | 2.00x |
| 5 | 20.000 | 20.010 | 20.010 | 20.010 | 20.010 | 5.00x |
| 10 | 10.000 | 10.009 | 10.009 | 10.009 | 10.009 | 9.99x |
| 20 | 5.000 | 5.010 | 5.010 | 5.010 | 5.010 | 19.96x |
| 50 | 2.000 | 2.035 | 3.077 | 2.275 | 2.462 | 40.62x |
| 100 | 1.000 | 2.066 | 2.050 | 2.067 | 2.061 | 48.53x |

The acceleration ratio is calculated as `T1 / Tp`, where `T1` is the average completion time with one worker and `Tp` is the average completion time with `p` workers. The measured time closely follows the ideal model from 1 through 20 workers. At 50 and 100 workers, the benefit saturates: connection acceptance, the listen backlog, thread scheduling, and client-launch overhead become significant relative to the one-second service time. One of the three 10-worker trials returned 99 successful responses and one empty reply, consistent with the separately detected `net.c` races; all other trials returned 100/100 HTTP 200 responses.

## 7. Conclusion

The bounded queue and thread-pool design demonstrated the required performance scaling, handled a workload larger than the queue capacity without deadlock, and enforced the 1-to-100 worker limit. However, the complete program is not yet ready for a truthful "no race conditions" claim because ThreadSanitizer found races in `net.c`. A final race-free rerun after correcting those calls, followed by compilation and a smoke test on Odin, is required.
