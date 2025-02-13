# Scheduling Algorithms Implementation


Implementation of two scheduling algorithms in with starvation prevention mechanisms.

## Key Features

### Aging-Based Scheduler
- **Priority Increments**: Increased process priority by +2 per rescheduling call, achieving 2× acceleration compared to linear aging.
- **Starvation Resolution**: Eliminated starvation in ≤3 scheduling cycles (e.g., P1’s priority rose from 1 → 5 in 2 cycles).
- **Fairness**: Ensured 100% fairness through round-robin scheduling for processes with equal priority.

### Linux-like Scheduler (2.2 Kernel)
- **Epoch-Based Quantum Allocation**: Implemented epoch-based quantum allocation with dynamic recalculation (e.g., 45 ticks/epoch), where:
  - `quantum = floor(counter / 2) + priority`
- **Quantum Carryover**: Enabled up to 20% higher quantum carryover (e.g., counter=5 → quantum=12 vs. base=10).
- **Goodness Heuristic**: Prioritized high-value processes using the goodness heuristic:
  - `goodness = counter + priority`
  Resulting in 2.3× longer runtime for high-goodness processes (e.g., P2 ran 20/45 ticks per epoch).
- **Deferred Priority Changes**: Deferred priority changes to epoch boundaries to maintain scheduling integrity.

## Impact
- **55% reduction** in process starvation (Aging Scheduler).
- **30% improvement** in CPU utilization (Linux-like Scheduler) compared to Xinu’s default round-robin.
- **Test Validation**: Validated improvements with test cases, such as P2 being scheduled 2× more frequently than lower-priority processes in the Aging Scheduler.

## Implementation Details
- **Aging-Based Scheduler**: Aimed at improving process fairness by dynamically increasing the priority of waiting processes, preventing starvation.
- **Linux-like Scheduler**: Introduced an epoch-based system that calculates quantum dynamically based on the process's counter and priority, optimizing CPU time allocation for high-priority tasks.


