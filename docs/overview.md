# Overview

ArkCompiler is a unified compilation and runtime platform that supports joint compilation and running across programming languages and chip platforms. It supports a variety of dynamic and static programming languages such as JS, TS, and ArkTS. It is the compilation and runtime base that enables OpenHarmony to run on multiple device forms such as mobile phones, PCs, tablets, TVs, automobiles, and wearables.

ArkCompiler consists of two parts: compiler toolchain and runtime.
The following figure shows the architecture of the compiler toolchain.

**Figure 1** Architecture of the compilation toolchain
![](figures/zh-cn_image_0000001197967897.png)

The compiler toolchain compiles ArkTS, TS, and JS source code into abc files, that is, ArkCompiler bytecode.
The following figure shows the architecture of the runtime.

**Figure 2** Architecture of the ArkCompiler eTS runtime

![](figures/zh-cn_image_ark-ts-arch.png)

The runtime runs the abc files to implement the corresponding semantic logic.

**ArkCompiler eTS Runtime consists of four subsystems**:

-   **Core Subsystem**

    Core Subsystem consists of the foundational runtime libraries that are language-agnostic, including the File component that carries bytecode, the Tooling component that supports Debugger functionality, and the Base library component responsible for adapting system calls.

-   **Execution Subsystem**

    Execution Subsystem consists of the interpreter for executing bytecode, the inline caching, and the profiler for capturing runtime information.

-   **Compiler Subsystem**

    Compiler Subsystem consists of the stub compiler, circuit framework, and Ahead-of-Time (AOT) compiler.

-   **Runtime subsystem**

    Runtime Subsystem contains modules related to the running of ArkTS, TS, and JS code.
    - Memory management: object allocator and garbage collector (CMS-GC and Partial-Compressing-GC for concurrent marking and partial memory compression)
    - Analysis tools: DFX, and profiling tools for CPU and heap analysis
    - Concurrency management: .abc file manager in the Actor model
    - Standard libraries: standard libraries defined by ECMAScript, efficient container libraries, and object models
    - Others: asynchronous work queues, TypeScript type loading, and ArkTS native APIs for interacting with C++ interfaces

**ArkCompiler features**:

- **Native type support**:<br>Unlike conventional engines that execute TS by first converting it to JS, the ArkCompiler compiler toolchain analyzes and deduces the TS type information when compiling the TS source code and transfers the information to the runtime. The runtime uses the TS type information to pre-generate inline caches before running, which speeds up bytecode execution. The TS AOT compiler directly compiles and generates machine code based on the TS type information in the .abc file, so that an application can directly run the optimized machine code, thereby greatly improving the running performance.

-  **Optimized concurrency model and concurrency APIs**:
The ECMAScript specification does not include concurrency semantic representation. Engines in the industry, such as browser or Node.js, usually provide the Worker APIs for multi-thread development based on the Actor concurrency model. In this model, executors do not share data objects. Instead, they communicate with each other using the messaging mechanism. The worker thread of the web engine or Node.js engine has defects such as slow startup and high memory usage.
To address these defects, the ArkCompiler eTS runtime supports sharing of immutable objects (methods and bytecode) in Actor instances, greatly optimizing Actor startup performance and startup memory.
In addition to the Worker APIs, the ArkCompiler eTS runtime provides TaskPool, a task pool function library that supports priority-based scheduling and automatic scaling of worker threads. With TaskPool, you do not need to care about the lifecycle of concurrent instances or create or destroy concurrent instances upon task load changes. This greatly simplifies the development of high-performance multithreaded OpenHarmony applications.

-  **Security**:<br>The ArkCompiler compiler toolchain statically precompiles ArkTS, TS, and JS code into Ark bytecode and enhances the multi-obfuscation capability, effectively improving the security of your code assets. For security purposes, ArkCompiler does not support JS code in sloppy mode or dynamic execution functions like eval.