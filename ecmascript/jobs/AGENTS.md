# ecmascript/jobs

Microtask queue implementation for Promise and async operation scheduling, conforming to ECMAScript microtask semantics.
Primary Language: C++

## Design

Implements the microtask queue for asynchronous operations:

```
  Promise.then() / queueMicrotask()
       │
       ▼
  MicroJobQueue::EnqueueMicrotask()
       │
       ├──▶ PendingJob (job wrapper)
       │     ├── Callback function
       │     ├── Arguments
       │     └── Context
       │
       ▼
  MicroJobQueue::Execute() (called at check points)
       │
       └──▶ Execute all queued jobs in FIFO order
```

**Key abstractions:**
- **MicroJobQueue** — Per-VM microtask queue, manages job lifecycle
- **PendingJob** — Job container holding callback and arguments
- **HiTraceScope** — Performance tracing integration

## Reference Source
- `arkcompiler/ets_runtime/ecmascript/jobs/micro_job_queue.h` (1-100行) - Microtask queue interface

## Key Files

| File | Role |
|------|------|
| `micro_job_queue.h/.cpp` | Microtask queue implementation |
| `pending_job.h` | Job container definition |
| `hitrace_scope.h/.cpp` | Performance tracing integration |

## Building

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.debug
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ark_js_host_linux_tools_packages
```

## Tests

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.debug unittest   # includes jobs tests
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/jobs/tests:host_unittest
```

## Boundaries

- Microtasks are per-VM; do not share queues across VMs.
- Jobs execute synchronously during `Execute()`; no async execution here.
- Integration with Promise and async/await is in `builtins/`.
