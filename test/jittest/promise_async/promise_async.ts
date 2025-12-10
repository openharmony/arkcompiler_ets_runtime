/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

declare function print(arg: any): string;
declare const ArkTools: {
    jitCompileAsync(args: any): boolean;
    waitJitCompileFinish(args: any): boolean;
};

// ==================== Promise Basic ====================

function testPromiseResolve(): string {
    let result = "";
    Promise.resolve(42).then(v => { result = `resolved:${v}`; });
    return "pending";
}

function testPromiseReject(): string {
    let result = "";
    Promise.reject("error").catch(e => { result = `caught:${e}`; });
    return "pending";
}

function testPromiseThen(): string {
    let result = "";
    Promise.resolve(1)
        .then(v => v + 1)
        .then(v => v * 2)
        .then(v => { result = `${v}`; });
    return "pending";
}

function testPromiseCatch(): string {
    let result = "";
    Promise.reject("err")
        .then(v => v)
        .catch(e => { result = `caught:${e}`; });
    return "pending";
}

function testPromiseFinally(): string {
    let result = "";
    Promise.resolve(42)
        .finally(() => { result += "finally,"; })
        .then(v => { result += `then:${v}`; });
    return "pending";
}

function testPromiseFinallyReject(): string {
    let result = "";
    Promise.reject("error")
        .finally(() => { result += "finally,"; })
        .catch(e => { result += `catch:${e}`; });
    return "pending";
}

function testPromiseConstructor(): string {
    let result = "";
    new Promise<number>((resolve, reject) => {
        resolve(100);
    }).then(v => { result = `${v}`; });
    return "pending";
}

function testPromiseConstructorReject(): string {
    let result = "";
    new Promise<number>((resolve, reject) => {
        reject("custom error");
    }).catch(e => { result = `${e}`; });
    return "pending";
}

// ==================== Promise Combinators ====================

function testPromiseAll(): string {
    let result = "";
    Promise.all([
        Promise.resolve(1),
        Promise.resolve(2),
        Promise.resolve(3)
    ]).then(values => { result = values.join(','); });
    return "pending";
}

function testPromiseAllReject(): string {
    let result = "";
    Promise.all([
        Promise.resolve(1),
        Promise.reject("error"),
        Promise.resolve(3)
    ]).catch(e => { result = `caught:${e}`; });
    return "pending";
}

function testPromiseAllEmpty(): string {
    let result = "";
    Promise.all([]).then(values => { result = `length:${values.length}`; });
    return "pending";
}

function testPromiseAllMixed(): string {
    let result = "";
    Promise.all([
        Promise.resolve(1),
        2,
        Promise.resolve(3)
    ]).then(values => { result = values.join(','); });
    return "pending";
}

function testPromiseRace(): string {
    let result = "";
    Promise.race([
        new Promise(r => setTimeout(() => r("slow"), 100)),
        Promise.resolve("fast")
    ]).then(v => { result = `${v}`; });
    return "pending";
}

function testPromiseRaceReject(): string {
    let result = "";
    Promise.race([
        new Promise((_, r) => setTimeout(() => r("slow"), 100)),
        Promise.reject("fast")
    ]).catch(e => { result = `${e}`; });
    return "pending";
}

function testPromiseAllSettled(): string {
    let result = "";
    Promise.allSettled([
        Promise.resolve(1),
        Promise.reject("error"),
        Promise.resolve(3)
    ]).then(results => {
        result = results.map(r => r.status).join(',');
    });
    return "pending";
}

function testPromiseAllSettledValues(): string {
    let result = "";
    Promise.allSettled([
        Promise.resolve(10),
        Promise.reject("err"),
        Promise.resolve(30)
    ]).then(results => {
        const values = results.map(r => {
            if (r.status === 'fulfilled') return r.value;
            return r.reason;
        });
        result = values.join(',');
    });
    return "pending";
}

function testPromiseAny(): string {
    let result = "";
    Promise.any([
        Promise.reject("err1"),
        Promise.resolve("success"),
        Promise.reject("err2")
    ]).then(v => { result = `${v}`; });
    return "pending";
}

function testPromiseAnyAllReject(): string {
    let result = "";
    Promise.any([
        Promise.reject("err1"),
        Promise.reject("err2"),
        Promise.reject("err3")
    ]).catch(e => { result = `AggregateError`; });
    return "pending";
}

// ==================== Promise Chaining ====================

function testPromiseChain(): string {
    let result = "";
    Promise.resolve(1)
        .then(v => Promise.resolve(v + 1))
        .then(v => Promise.resolve(v * 2))
        .then(v => { result = `${v}`; });
    return "pending";
}

function testPromiseNestedThen(): string {
    let result = "";
    Promise.resolve(1).then(v => {
        return Promise.resolve(2).then(v2 => {
            return v + v2;
        });
    }).then(v => { result = `${v}`; });
    return "pending";
}

function testPromiseLongChain(): string {
    let result = "";
    Promise.resolve(1)
        .then(v => v + 1)
        .then(v => v + 1)
        .then(v => v + 1)
        .then(v => v + 1)
        .then(v => v + 1)
        .then(v => { result = `${v}`; });
    return "pending";
}

function testPromiseChainWithCatch(): string {
    let result = "";
    Promise.resolve(1)
        .then(v => { throw new Error("error"); })
        .catch(e => 100)
        .then(v => { result = `recovered:${v}`; });
    return "pending";
}

function testPromiseChainMultipleCatch(): string {
    let result = "";
    Promise.reject("err1")
        .catch(e => { throw "err2"; })
        .catch(e => { throw "err3"; })
        .catch(e => { result = `final:${e}`; });
    return "pending";
}

// ==================== Promise Static Methods ====================

function testPromiseResolveValue(): string {
    let result = "";
    Promise.resolve(Promise.resolve(42)).then(v => { result = `${v}`; });
    return "pending";
}

function testPromiseResolveThenable(): string {
    let result = "";
    const thenable = {
        then(resolve: (v: number) => void) {
            resolve(99);
        }
    };
    Promise.resolve(thenable).then(v => { result = `${v}`; });
    return "pending";
}

// ==================== Async/Await Basic ====================

async function asyncBasic(): Promise<string> {
    const value = await Promise.resolve(42);
    return `result:${value}`;
}

async function asyncMultipleAwait(): Promise<string> {
    const a = await Promise.resolve(1);
    const b = await Promise.resolve(2);
    const c = await Promise.resolve(3);
    return `${a},${b},${c}`;
}

async function asyncSequential(): Promise<string> {
    let result = "";
    result += await Promise.resolve("A");
    result += await Promise.resolve("B");
    result += await Promise.resolve("C");
    return result;
}

async function asyncReturnValue(): Promise<number> {
    return 42;
}

async function asyncReturnPromise(): Promise<number> {
    return Promise.resolve(100);
}

async function asyncAwaitNonPromise(): Promise<string> {
    const value = await 42;
    return `non-promise:${value}`;
}

async function asyncParallel(): Promise<string> {
    const [a, b, c] = await Promise.all([
        Promise.resolve(1),
        Promise.resolve(2),
        Promise.resolve(3)
    ]);
    return `${a},${b},${c}`;
}

async function asyncConditional(): Promise<string> {
    const flag = await Promise.resolve(true);
    if (flag) {
        return await Promise.resolve("true branch");
    } else {
        return await Promise.resolve("false branch");
    }
}

// ==================== Async Error Handling ====================

async function asyncTryCatch(): Promise<string> {
    try {
        await Promise.reject("async error");
        return "no error";
    } catch (e) {
        return `caught:${e}`;
    }
}

async function asyncFinallyBlock(): Promise<string> {
    let result = "";
    try {
        result = await Promise.resolve("value");
    } finally {
        result += ",finally";
    }
    return result;
}

async function asyncNestedTryCatch(): Promise<string> {
    try {
        try {
            await Promise.reject("inner");
        } catch (e) {
            throw `rethrow:${e}`;
        }
    } catch (e) {
        return `outer:${e}`;
    }
    return "unreached";
}

async function asyncTryCatchFinally(): Promise<string> {
    let result = "";
    try {
        await Promise.reject("error");
    } catch (e) {
        result += `catch:${e}`;
    } finally {
        result += ",finally";
    }
    return result;
}

async function asyncMultipleTryCatch(): Promise<string> {
    let result = "";
    try {
        await Promise.reject("first");
    } catch (e) {
        result += `first:${e},`;
    }
    try {
        await Promise.reject("second");
    } catch (e) {
        result += `second:${e}`;
    }
    return result;
}

async function asyncThrowInCatch(): Promise<string> {
    try {
        try {
            await Promise.reject("original");
        } catch (e) {
            await Promise.reject("new error");
        }
    } catch (e) {
        return `final:${e}`;
    }
    return "unreached";
}

// ==================== Async Loops ====================

async function asyncForLoop(): Promise<string> {
    const results: number[] = [];
    for (let i = 0; i < 3; i++) {
        const val = await Promise.resolve(i * 2);
        results.push(val);
    }
    return results.join(',');
}

async function asyncWhileLoop(): Promise<string> {
    const results: number[] = [];
    let i = 0;
    while (i < 3) {
        const val = await Promise.resolve(i * 10);
        results.push(val);
        i++;
    }
    return results.join(',');
}

async function asyncForOfLoop(): Promise<string> {
    const items = [1, 2, 3];
    const results: number[] = [];
    for (const item of items) {
        const doubled = await Promise.resolve(item * 2);
        results.push(doubled);
    }
    return results.join(',');
}

async function asyncMapSequential(): Promise<string> {
    const items = [1, 2, 3];
    const results: number[] = [];
    for (const item of items) {
        const doubled = await Promise.resolve(item * 2);
        results.push(doubled);
    }
    return results.join(',');
}

async function asyncMapParallel(): Promise<string> {
    const items = [1, 2, 3];
    const promises = items.map(item => Promise.resolve(item * 2));
    const results = await Promise.all(promises);
    return results.join(',');
}

async function asyncReduceSequential(): Promise<string> {
    const items = [1, 2, 3, 4, 5];
    let sum = 0;
    for (const item of items) {
        sum += await Promise.resolve(item);
    }
    return `sum:${sum}`;
}

async function asyncBreakInLoop(): Promise<string> {
    const results: number[] = [];
    for (let i = 0; i < 10; i++) {
        if (i >= 3) break;
        const val = await Promise.resolve(i);
        results.push(val);
    }
    return results.join(',');
}

async function asyncContinueInLoop(): Promise<string> {
    const results: number[] = [];
    for (let i = 0; i < 5; i++) {
        if (i % 2 === 0) continue;
        const val = await Promise.resolve(i);
        results.push(val);
    }
    return results.join(',');
}

// ==================== Async Class ====================

class AsyncClass {
    value: number;
    constructor(value: number) {
        this.value = value;
    }
    async getValue(): Promise<number> {
        return await Promise.resolve(this.value);
    }
    async doubleValue(): Promise<number> {
        const val = await this.getValue();
        return val * 2;
    }
    async add(n: number): Promise<number> {
        return await Promise.resolve(this.value + n);
    }
}

async function testAsyncClass(): Promise<string> {
    const obj = new AsyncClass(21);
    const val = await obj.getValue();
    const doubled = await obj.doubleValue();
    return `${val},${doubled}`;
}

class AsyncClassStatic {
    static async create(value: number): Promise<AsyncClassStatic> {
        await Promise.resolve();
        return new AsyncClassStatic(value);
    }
    constructor(public value: number) {}
    async getValue(): Promise<number> {
        return this.value;
    }
}

async function testAsyncClassStatic(): Promise<string> {
    const obj = await AsyncClassStatic.create(50);
    const val = await obj.getValue();
    return `${val}`;
}

class AsyncClassInheritance {
    async baseMethod(): Promise<string> {
        return "base";
    }
}

class AsyncClassChild extends AsyncClassInheritance {
    async baseMethod(): Promise<string> {
        const base = await super.baseMethod();
        return `${base}:child`;
    }
}

async function testAsyncClassInheritance(): Promise<string> {
    const obj = new AsyncClassChild();
    return await obj.baseMethod();
}

// ==================== Async Arrow Functions ====================

const asyncArrow = async (): Promise<string> => {
    const val = await Promise.resolve(100);
    return `arrow:${val}`;
};

const asyncArrowParam = async (x: number): Promise<number> => {
    return await Promise.resolve(x * 3);
};

const asyncArrowMultiParam = async (a: number, b: number): Promise<number> => {
    const va = await Promise.resolve(a);
    const vb = await Promise.resolve(b);
    return va + vb;
};

async function testAsyncArrow(): Promise<string> {
    const r1 = await asyncArrow();
    const r2 = await asyncArrowParam(10);
    return `${r1},${r2}`;
}

async function testAsyncArrowMulti(): Promise<string> {
    const result = await asyncArrowMultiParam(5, 7);
    return `${result}`;
}

// ==================== Async IIFE ====================

async function testAsyncIIFE(): Promise<string> {
    const result = await (async () => {
        return await Promise.resolve("iife");
    })();
    return result;
}

// ==================== Async Generator Pattern ====================

async function* asyncGeneratorSimple(): AsyncGenerator<number> {
    yield await Promise.resolve(1);
    yield await Promise.resolve(2);
    yield await Promise.resolve(3);
}

async function testAsyncGenerator(): Promise<string> {
    const results: number[] = [];
    for await (const val of asyncGeneratorSimple()) {
        results.push(val);
    }
    return results.join(',');
}

// ==================== Promise Utilities ====================

function delay(ms: number): Promise<void> {
    return new Promise(resolve => setTimeout(resolve, ms));
}

async function testDelay(): Promise<string> {
    const start = Date.now();
    await delay(10);
    const elapsed = Date.now() - start;
    return `delayed:${elapsed >= 0}`;
}

function timeout<T>(promise: Promise<T>, ms: number): Promise<T> {
    return Promise.race([
        promise,
        new Promise<T>((_, reject) =>
            setTimeout(() => reject(new Error("timeout")), ms)
        )
    ]);
}

async function testTimeout(): Promise<string> {
    try {
        await timeout(Promise.resolve("fast"), 100);
        return "resolved";
    } catch (e) {
        return "timeout";
    }
}

async function retry<T>(fn: () => Promise<T>, attempts: number): Promise<T> {
    for (let i = 0; i < attempts; i++) {
        try {
            return await fn();
        } catch (e) {
            if (i === attempts - 1) throw e;
        }
    }
    throw new Error("unreachable");
}

async function testRetry(): Promise<string> {
    let count = 0;
    const result = await retry(async () => {
        count++;
        if (count < 3) throw new Error("fail");
        return "success";
    }, 5);
    return `${result}:${count}`;
}

// Synchronous test wrappers for JIT
function syncTestPromiseResolve(): string { return testPromiseResolve(); }
function syncTestPromiseReject(): string { return testPromiseReject(); }
function syncTestPromiseThen(): string { return testPromiseThen(); }
function syncTestPromiseCatch(): string { return testPromiseCatch(); }
function syncTestPromiseFinally(): string { return testPromiseFinally(); }
function syncTestPromiseFinallyReject(): string { return testPromiseFinallyReject(); }
function syncTestPromiseConstructor(): string { return testPromiseConstructor(); }
function syncTestPromiseConstructorReject(): string { return testPromiseConstructorReject(); }
function syncTestPromiseAll(): string { return testPromiseAll(); }
function syncTestPromiseAllReject(): string { return testPromiseAllReject(); }
function syncTestPromiseAllEmpty(): string { return testPromiseAllEmpty(); }
function syncTestPromiseAllMixed(): string { return testPromiseAllMixed(); }
function syncTestPromiseRace(): string { return testPromiseRace(); }
function syncTestPromiseRaceReject(): string { return testPromiseRaceReject(); }
function syncTestPromiseAllSettled(): string { return testPromiseAllSettled(); }
function syncTestPromiseAllSettledValues(): string { return testPromiseAllSettledValues(); }
function syncTestPromiseAny(): string { return testPromiseAny(); }
function syncTestPromiseAnyAllReject(): string { return testPromiseAnyAllReject(); }
function syncTestPromiseChain(): string { return testPromiseChain(); }
function syncTestPromiseNestedThen(): string { return testPromiseNestedThen(); }
function syncTestPromiseLongChain(): string { return testPromiseLongChain(); }
function syncTestPromiseChainWithCatch(): string { return testPromiseChainWithCatch(); }
function syncTestPromiseChainMultipleCatch(): string { return testPromiseChainMultipleCatch(); }
function syncTestPromiseResolveValue(): string { return testPromiseResolveValue(); }
function syncTestPromiseResolveThenable(): string { return testPromiseResolveThenable(); }

// Warmup
for (let i = 0; i < 20; i++) {
    syncTestPromiseResolve();
    syncTestPromiseReject();
    syncTestPromiseThen();
    syncTestPromiseCatch();
    syncTestPromiseFinally();
    syncTestPromiseFinallyReject();
    syncTestPromiseConstructor();
    syncTestPromiseConstructorReject();
    syncTestPromiseAll();
    syncTestPromiseAllReject();
    syncTestPromiseAllEmpty();
    syncTestPromiseAllMixed();
    syncTestPromiseRace();
    syncTestPromiseRaceReject();
    syncTestPromiseAllSettled();
    syncTestPromiseAllSettledValues();
    syncTestPromiseAny();
    syncTestPromiseAnyAllReject();
    syncTestPromiseChain();
    syncTestPromiseNestedThen();
    syncTestPromiseLongChain();
    syncTestPromiseChainWithCatch();
    syncTestPromiseChainMultipleCatch();
    syncTestPromiseResolveValue();
    syncTestPromiseResolveThenable();
    asyncBasic();
    asyncMultipleAwait();
    asyncSequential();
    asyncReturnValue();
    asyncReturnPromise();
    asyncAwaitNonPromise();
    asyncParallel();
    asyncConditional();
    asyncTryCatch();
    asyncFinallyBlock();
    asyncNestedTryCatch();
    asyncTryCatchFinally();
    asyncMultipleTryCatch();
    asyncThrowInCatch();
    asyncForLoop();
    asyncWhileLoop();
    asyncForOfLoop();
    asyncMapSequential();
    asyncMapParallel();
    asyncReduceSequential();
    asyncBreakInLoop();
    asyncContinueInLoop();
    testAsyncClass();
    testAsyncClassStatic();
    testAsyncClassInheritance();
    testAsyncArrow();
    testAsyncArrowMulti();
    testAsyncIIFE();
    testAsyncGenerator();
    testDelay();
    testTimeout();
    testRetry();
}

// JIT compile
ArkTools.jitCompileAsync(syncTestPromiseResolve);
ArkTools.jitCompileAsync(syncTestPromiseReject);
ArkTools.jitCompileAsync(syncTestPromiseThen);
ArkTools.jitCompileAsync(syncTestPromiseCatch);
ArkTools.jitCompileAsync(syncTestPromiseFinally);
ArkTools.jitCompileAsync(syncTestPromiseFinallyReject);
ArkTools.jitCompileAsync(syncTestPromiseConstructor);
ArkTools.jitCompileAsync(syncTestPromiseConstructorReject);
ArkTools.jitCompileAsync(syncTestPromiseAll);
ArkTools.jitCompileAsync(syncTestPromiseAllReject);
ArkTools.jitCompileAsync(syncTestPromiseAllEmpty);
ArkTools.jitCompileAsync(syncTestPromiseAllMixed);
ArkTools.jitCompileAsync(syncTestPromiseRace);
ArkTools.jitCompileAsync(syncTestPromiseRaceReject);
ArkTools.jitCompileAsync(syncTestPromiseAllSettled);
ArkTools.jitCompileAsync(syncTestPromiseAllSettledValues);
ArkTools.jitCompileAsync(syncTestPromiseAny);
ArkTools.jitCompileAsync(syncTestPromiseAnyAllReject);
ArkTools.jitCompileAsync(syncTestPromiseChain);
ArkTools.jitCompileAsync(syncTestPromiseNestedThen);
ArkTools.jitCompileAsync(syncTestPromiseLongChain);
ArkTools.jitCompileAsync(syncTestPromiseChainWithCatch);
ArkTools.jitCompileAsync(syncTestPromiseChainMultipleCatch);
ArkTools.jitCompileAsync(syncTestPromiseResolveValue);
ArkTools.jitCompileAsync(syncTestPromiseResolveThenable);
ArkTools.jitCompileAsync(asyncBasic);
ArkTools.jitCompileAsync(asyncMultipleAwait);
ArkTools.jitCompileAsync(asyncSequential);
ArkTools.jitCompileAsync(asyncReturnValue);
ArkTools.jitCompileAsync(asyncReturnPromise);
ArkTools.jitCompileAsync(asyncAwaitNonPromise);
ArkTools.jitCompileAsync(asyncParallel);
ArkTools.jitCompileAsync(asyncConditional);
ArkTools.jitCompileAsync(asyncTryCatch);
ArkTools.jitCompileAsync(asyncFinallyBlock);
ArkTools.jitCompileAsync(asyncNestedTryCatch);
ArkTools.jitCompileAsync(asyncTryCatchFinally);
ArkTools.jitCompileAsync(asyncMultipleTryCatch);
ArkTools.jitCompileAsync(asyncThrowInCatch);
ArkTools.jitCompileAsync(asyncForLoop);
ArkTools.jitCompileAsync(asyncWhileLoop);
ArkTools.jitCompileAsync(asyncForOfLoop);
ArkTools.jitCompileAsync(asyncMapSequential);
ArkTools.jitCompileAsync(asyncMapParallel);
ArkTools.jitCompileAsync(asyncReduceSequential);
ArkTools.jitCompileAsync(asyncBreakInLoop);
ArkTools.jitCompileAsync(asyncContinueInLoop);
ArkTools.jitCompileAsync(testAsyncClass);
ArkTools.jitCompileAsync(testAsyncClassStatic);
ArkTools.jitCompileAsync(testAsyncClassInheritance);
ArkTools.jitCompileAsync(testAsyncArrow);
ArkTools.jitCompileAsync(testAsyncArrowMulti);
ArkTools.jitCompileAsync(testAsyncIIFE);
ArkTools.jitCompileAsync(testAsyncGenerator);
ArkTools.jitCompileAsync(testDelay);
ArkTools.jitCompileAsync(testTimeout);
ArkTools.jitCompileAsync(testRetry);

print("compile syncTestPromiseResolve: " + ArkTools.waitJitCompileFinish(syncTestPromiseResolve));
print("compile syncTestPromiseReject: " + ArkTools.waitJitCompileFinish(syncTestPromiseReject));
print("compile syncTestPromiseThen: " + ArkTools.waitJitCompileFinish(syncTestPromiseThen));
print("compile syncTestPromiseCatch: " + ArkTools.waitJitCompileFinish(syncTestPromiseCatch));
print("compile syncTestPromiseFinally: " + ArkTools.waitJitCompileFinish(syncTestPromiseFinally));
print("compile syncTestPromiseFinallyReject: " + ArkTools.waitJitCompileFinish(syncTestPromiseFinallyReject));
print("compile syncTestPromiseConstructor: " + ArkTools.waitJitCompileFinish(syncTestPromiseConstructor));
print("compile syncTestPromiseConstructorReject: " + ArkTools.waitJitCompileFinish(syncTestPromiseConstructorReject));
print("compile syncTestPromiseAll: " + ArkTools.waitJitCompileFinish(syncTestPromiseAll));
print("compile syncTestPromiseAllReject: " + ArkTools.waitJitCompileFinish(syncTestPromiseAllReject));
print("compile syncTestPromiseAllEmpty: " + ArkTools.waitJitCompileFinish(syncTestPromiseAllEmpty));
print("compile syncTestPromiseAllMixed: " + ArkTools.waitJitCompileFinish(syncTestPromiseAllMixed));
print("compile syncTestPromiseRace: " + ArkTools.waitJitCompileFinish(syncTestPromiseRace));
print("compile syncTestPromiseRaceReject: " + ArkTools.waitJitCompileFinish(syncTestPromiseRaceReject));
print("compile syncTestPromiseAllSettled: " + ArkTools.waitJitCompileFinish(syncTestPromiseAllSettled));
print("compile syncTestPromiseAllSettledValues: " + ArkTools.waitJitCompileFinish(syncTestPromiseAllSettledValues));
print("compile syncTestPromiseAny: " + ArkTools.waitJitCompileFinish(syncTestPromiseAny));
print("compile syncTestPromiseAnyAllReject: " + ArkTools.waitJitCompileFinish(syncTestPromiseAnyAllReject));
print("compile syncTestPromiseChain: " + ArkTools.waitJitCompileFinish(syncTestPromiseChain));
print("compile syncTestPromiseNestedThen: " + ArkTools.waitJitCompileFinish(syncTestPromiseNestedThen));
print("compile syncTestPromiseLongChain: " + ArkTools.waitJitCompileFinish(syncTestPromiseLongChain));
print("compile syncTestPromiseChainWithCatch: " + ArkTools.waitJitCompileFinish(syncTestPromiseChainWithCatch));
print("compile syncTestPromiseChainMultipleCatch: " + ArkTools.waitJitCompileFinish(syncTestPromiseChainMultipleCatch));
print("compile syncTestPromiseResolveValue: " + ArkTools.waitJitCompileFinish(syncTestPromiseResolveValue));
print("compile syncTestPromiseResolveThenable: " + ArkTools.waitJitCompileFinish(syncTestPromiseResolveThenable));
print("compile asyncBasic: " + ArkTools.waitJitCompileFinish(asyncBasic));
print("compile asyncMultipleAwait: " + ArkTools.waitJitCompileFinish(asyncMultipleAwait));
print("compile asyncSequential: " + ArkTools.waitJitCompileFinish(asyncSequential));
print("compile asyncReturnValue: " + ArkTools.waitJitCompileFinish(asyncReturnValue));
print("compile asyncReturnPromise: " + ArkTools.waitJitCompileFinish(asyncReturnPromise));
print("compile asyncAwaitNonPromise: " + ArkTools.waitJitCompileFinish(asyncAwaitNonPromise));
print("compile asyncParallel: " + ArkTools.waitJitCompileFinish(asyncParallel));
print("compile asyncConditional: " + ArkTools.waitJitCompileFinish(asyncConditional));
print("compile asyncTryCatch: " + ArkTools.waitJitCompileFinish(asyncTryCatch));
print("compile asyncFinallyBlock: " + ArkTools.waitJitCompileFinish(asyncFinallyBlock));
print("compile asyncNestedTryCatch: " + ArkTools.waitJitCompileFinish(asyncNestedTryCatch));
print("compile asyncTryCatchFinally: " + ArkTools.waitJitCompileFinish(asyncTryCatchFinally));
print("compile asyncMultipleTryCatch: " + ArkTools.waitJitCompileFinish(asyncMultipleTryCatch));
print("compile asyncThrowInCatch: " + ArkTools.waitJitCompileFinish(asyncThrowInCatch));
print("compile asyncForLoop: " + ArkTools.waitJitCompileFinish(asyncForLoop));
print("compile asyncWhileLoop: " + ArkTools.waitJitCompileFinish(asyncWhileLoop));
print("compile asyncForOfLoop: " + ArkTools.waitJitCompileFinish(asyncForOfLoop));
print("compile asyncMapSequential: " + ArkTools.waitJitCompileFinish(asyncMapSequential));
print("compile asyncMapParallel: " + ArkTools.waitJitCompileFinish(asyncMapParallel));
print("compile asyncReduceSequential: " + ArkTools.waitJitCompileFinish(asyncReduceSequential));
print("compile asyncBreakInLoop: " + ArkTools.waitJitCompileFinish(asyncBreakInLoop));
print("compile asyncContinueInLoop: " + ArkTools.waitJitCompileFinish(asyncContinueInLoop));
print("compile testAsyncClass: " + ArkTools.waitJitCompileFinish(testAsyncClass));
print("compile testAsyncClassStatic: " + ArkTools.waitJitCompileFinish(testAsyncClassStatic));
print("compile testAsyncClassInheritance: " + ArkTools.waitJitCompileFinish(testAsyncClassInheritance));
print("compile testAsyncArrow: " + ArkTools.waitJitCompileFinish(testAsyncArrow));
print("compile testAsyncArrowMulti: " + ArkTools.waitJitCompileFinish(testAsyncArrowMulti));
print("compile testAsyncIIFE: " + ArkTools.waitJitCompileFinish(testAsyncIIFE));
print("compile testAsyncGenerator: " + ArkTools.waitJitCompileFinish(testAsyncGenerator));
print("compile testDelay: " + ArkTools.waitJitCompileFinish(testDelay));
print("compile testTimeout: " + ArkTools.waitJitCompileFinish(testTimeout));
print("compile testRetry: " + ArkTools.waitJitCompileFinish(testRetry));

// Verify sync functions
print("syncTestPromiseResolve: " + syncTestPromiseResolve());
print("syncTestPromiseReject: " + syncTestPromiseReject());
print("syncTestPromiseThen: " + syncTestPromiseThen());
print("syncTestPromiseCatch: " + syncTestPromiseCatch());
print("syncTestPromiseFinally: " + syncTestPromiseFinally());
print("syncTestPromiseFinallyReject: " + syncTestPromiseFinallyReject());
print("syncTestPromiseConstructor: " + syncTestPromiseConstructor());
print("syncTestPromiseConstructorReject: " + syncTestPromiseConstructorReject());
print("syncTestPromiseAll: " + syncTestPromiseAll());
print("syncTestPromiseAllReject: " + syncTestPromiseAllReject());
print("syncTestPromiseAllEmpty: " + syncTestPromiseAllEmpty());
print("syncTestPromiseAllMixed: " + syncTestPromiseAllMixed());
print("syncTestPromiseRace: " + syncTestPromiseRace());
print("syncTestPromiseRaceReject: " + syncTestPromiseRaceReject());
print("syncTestPromiseAllSettled: " + syncTestPromiseAllSettled());
print("syncTestPromiseAllSettledValues: " + syncTestPromiseAllSettledValues());
print("syncTestPromiseAny: " + syncTestPromiseAny());
print("syncTestPromiseAnyAllReject: " + syncTestPromiseAnyAllReject());
print("syncTestPromiseChain: " + syncTestPromiseChain());
print("syncTestPromiseNestedThen: " + syncTestPromiseNestedThen());
print("syncTestPromiseLongChain: " + syncTestPromiseLongChain());
print("syncTestPromiseChainWithCatch: " + syncTestPromiseChainWithCatch());
print("syncTestPromiseChainMultipleCatch: " + syncTestPromiseChainMultipleCatch());
print("syncTestPromiseResolveValue: " + syncTestPromiseResolveValue());
print("syncTestPromiseResolveThenable: " + syncTestPromiseResolveThenable());
