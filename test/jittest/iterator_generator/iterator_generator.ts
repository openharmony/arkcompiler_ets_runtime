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

// ==================== Basic Generator ====================

function* simpleGenerator(): Generator<number> {
    yield 1;
    yield 2;
    yield 3;
}

function testSimpleGenerator(): string {
    const gen = simpleGenerator();
    const a = gen.next();
    const b = gen.next();
    const c = gen.next();
    const d = gen.next();
    return `${a.value},${a.done},${b.value},${b.done},${c.value},${c.done},${d.value},${d.done}`;
}

function* generatorWithReturn(): Generator<number, string> {
    yield 1;
    yield 2;
    return 'done';
}

function testGeneratorWithReturn(): string {
    const gen = generatorWithReturn();
    const results: string[] = [];
    let result = gen.next();
    while (!result.done) {
        results.push(String(result.value));
        result = gen.next();
    }
    results.push(String(result.value));
    return results.join(',');
}

function* generatorWithParams(start: number, end: number): Generator<number> {
    for (let i = start; i <= end; i++) {
        yield i;
    }
}

function testGeneratorWithParams(): string {
    const gen = generatorWithParams(5, 8);
    return [...gen].join(',');
}

// ==================== Generator Control Flow ====================

function* yieldExpression(): Generator<number, void, number> {
    const x = yield 1;
    const y = yield x + 10;
    yield y + 100;
}

function testYieldExpression(): string {
    const gen = yieldExpression();
    const a = gen.next();
    const b = gen.next(5);
    const c = gen.next(20);
    return `${a.value},${b.value},${c.value}`;
}

function* generatorThrow(): Generator<number> {
    try {
        yield 1;
        yield 2;
    } catch (e) {
        yield 100;
    }
    yield 3;
}

function testGeneratorThrow(): string {
    const gen = generatorThrow();
    const a = gen.next();
    const b = gen.throw(new Error('test'));
    const c = gen.next();
    return `${a.value},${b.value},${c.value}`;
}

function* generatorReturn(): Generator<number> {
    yield 1;
    yield 2;
    yield 3;
}

function testGeneratorReturn(): string {
    const gen = generatorReturn();
    const a = gen.next();
    const b = gen.return(99);
    const c = gen.next();
    return `${a.value},${b.value},${b.done},${c.value},${c.done}`;
}

// ==================== Delegating Generators ====================

function* inner(): Generator<number> {
    yield 2;
    yield 3;
}

function* outer(): Generator<number> {
    yield 1;
    yield* inner();
    yield 4;
}

function testDelegatingGenerator(): string {
    return [...outer()].join(',');
}

function* delegateToArray(): Generator<number> {
    yield 0;
    yield* [1, 2, 3];
    yield 4;
}

function testDelegateToArray(): string {
    return [...delegateToArray()].join(',');
}

function* delegateToString(): Generator<string> {
    yield 'start';
    yield* 'abc';
    yield 'end';
}

function testDelegateToString(): string {
    return [...delegateToString()].join(',');
}

// ==================== Infinite Generators ====================

function* infiniteSequence(): Generator<number> {
    let n = 0;
    while (true) {
        yield n++;
    }
}

function testInfiniteSequence(): string {
    const gen = infiniteSequence();
    const results: number[] = [];
    for (let i = 0; i < 5; i++) {
        results.push(gen.next().value as number);
    }
    return results.join(',');
}

function* fibonacci(): Generator<number> {
    let [a, b] = [0, 1];
    while (true) {
        yield a;
        [a, b] = [b, a + b];
    }
}

function testFibonacci(): string {
    const gen = fibonacci();
    const results: number[] = [];
    for (let i = 0; i < 8; i++) {
        results.push(gen.next().value as number);
    }
    return results.join(',');
}

// ==================== Generator as Iterator ====================

function testGeneratorForOf(): string {
    const results: number[] = [];
    for (const val of simpleGenerator()) {
        results.push(val);
    }
    return results.join(',');
}

function testGeneratorSpread(): string {
    const arr = [...simpleGenerator()];
    return arr.join(',');
}

function testGeneratorDestructuring(): string {
    const [a, b, c] = simpleGenerator();
    return `${a},${b},${c}`;
}

// ==================== Custom Iterables ====================

const customIterable = {
    data: [10, 20, 30],
    [Symbol.iterator](): Iterator<number> {
        let index = 0;
        const data = this.data;
        return {
            next(): IteratorResult<number> {
                if (index < data.length) {
                    return { value: data[index++], done: false };
                }
                return { value: undefined as any, done: true };
            }
        };
    }
};

function testCustomIterable(): string {
    return [...customIterable].join(',');
}

class Range {
    constructor(private start: number, private end: number) {}

    *[Symbol.iterator](): Generator<number> {
        for (let i = this.start; i <= this.end; i++) {
            yield i;
        }
    }
}

function testRangeIterator(): string {
    const range = new Range(1, 5);
    return [...range].join(',');
}

// ==================== Iterator Helpers ====================

function* take<T>(iterable: Iterable<T>, n: number): Generator<T> {
    let count = 0;
    for (const item of iterable) {
        if (count >= n) break;
        yield item;
        count++;
    }
}

function testTakeHelper(): string {
    const numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    return [...take(numbers, 4)].join(',');
}

function* filter<T>(iterable: Iterable<T>, predicate: (item: T) => boolean): Generator<T> {
    for (const item of iterable) {
        if (predicate(item)) {
            yield item;
        }
    }
}

function testFilterHelper(): string {
    const numbers = [1, 2, 3, 4, 5, 6];
    return [...filter(numbers, n => n % 2 === 0)].join(',');
}

function* map<T, U>(iterable: Iterable<T>, fn: (item: T) => U): Generator<U> {
    for (const item of iterable) {
        yield fn(item);
    }
}

function testMapHelper(): string {
    const numbers = [1, 2, 3];
    return [...map(numbers, n => n * 10)].join(',');
}

function* zip<T, U>(a: Iterable<T>, b: Iterable<U>): Generator<[T, U]> {
    const iterA = a[Symbol.iterator]();
    const iterB = b[Symbol.iterator]();
    while (true) {
        const resultA = iterA.next();
        const resultB = iterB.next();
        if (resultA.done || resultB.done) break;
        yield [resultA.value, resultB.value];
    }
}

function testZipHelper(): string {
    const a = [1, 2, 3];
    const b = ['a', 'b', 'c'];
    return [...zip(a, b)].map(([x, y]) => `${x}${y}`).join(',');
}

// ==================== Generator Composition ====================

function* chain<T>(...iterables: Iterable<T>[]): Generator<T> {
    for (const iterable of iterables) {
        yield* iterable;
    }
}

function testChainGenerator(): string {
    return [...chain([1, 2], [3, 4], [5, 6])].join(',');
}

function* flattenOnce<T>(iterables: Iterable<Iterable<T>>): Generator<T> {
    for (const iterable of iterables) {
        yield* iterable;
    }
}

function testFlattenGenerator(): string {
    const nested = [[1, 2], [3, 4], [5, 6]];
    return [...flattenOnce(nested)].join(',');
}

// ==================== Async-like Patterns with Generators ====================

function* stateMachine(): Generator<string, void, string> {
    let state = 'idle';
    while (true) {
        const event = yield state;
        switch (state) {
            case 'idle':
                if (event === 'start') state = 'running';
                break;
            case 'running':
                if (event === 'pause') state = 'paused';
                if (event === 'stop') state = 'idle';
                break;
            case 'paused':
                if (event === 'resume') state = 'running';
                if (event === 'stop') state = 'idle';
                break;
        }
    }
}

function testStateMachine(): string {
    const sm = stateMachine();
    const results: string[] = [];
    results.push(sm.next().value as string);
    results.push(sm.next('start').value as string);
    results.push(sm.next('pause').value as string);
    results.push(sm.next('resume').value as string);
    results.push(sm.next('stop').value as string);
    return results.join(',');
}

// Warmup
for (let i = 0; i < 20; i++) {
    testSimpleGenerator();
    testGeneratorWithReturn();
    testGeneratorWithParams();
    testYieldExpression();
    testGeneratorThrow();
    testGeneratorReturn();
    testDelegatingGenerator();
    testDelegateToArray();
    testDelegateToString();
    testInfiniteSequence();
    testFibonacci();
    testGeneratorForOf();
    testGeneratorSpread();
    testGeneratorDestructuring();
    testCustomIterable();
    testRangeIterator();
    testTakeHelper();
    testFilterHelper();
    testMapHelper();
    testZipHelper();
    testChainGenerator();
    testFlattenGenerator();
    testStateMachine();
}

// JIT compile
ArkTools.jitCompileAsync(testSimpleGenerator);
ArkTools.jitCompileAsync(testGeneratorWithReturn);
ArkTools.jitCompileAsync(testGeneratorWithParams);
ArkTools.jitCompileAsync(testYieldExpression);
ArkTools.jitCompileAsync(testGeneratorThrow);
ArkTools.jitCompileAsync(testGeneratorReturn);
ArkTools.jitCompileAsync(testDelegatingGenerator);
ArkTools.jitCompileAsync(testDelegateToArray);
ArkTools.jitCompileAsync(testDelegateToString);
ArkTools.jitCompileAsync(testInfiniteSequence);
ArkTools.jitCompileAsync(testFibonacci);
ArkTools.jitCompileAsync(testGeneratorForOf);
ArkTools.jitCompileAsync(testGeneratorSpread);
ArkTools.jitCompileAsync(testGeneratorDestructuring);
ArkTools.jitCompileAsync(testCustomIterable);
ArkTools.jitCompileAsync(testRangeIterator);
ArkTools.jitCompileAsync(testTakeHelper);
ArkTools.jitCompileAsync(testFilterHelper);
ArkTools.jitCompileAsync(testMapHelper);
ArkTools.jitCompileAsync(testZipHelper);
ArkTools.jitCompileAsync(testChainGenerator);
ArkTools.jitCompileAsync(testFlattenGenerator);
ArkTools.jitCompileAsync(testStateMachine);

print("compile testSimpleGenerator: " + ArkTools.waitJitCompileFinish(testSimpleGenerator));
print("compile testGeneratorWithReturn: " + ArkTools.waitJitCompileFinish(testGeneratorWithReturn));
print("compile testGeneratorWithParams: " + ArkTools.waitJitCompileFinish(testGeneratorWithParams));
print("compile testYieldExpression: " + ArkTools.waitJitCompileFinish(testYieldExpression));
print("compile testGeneratorThrow: " + ArkTools.waitJitCompileFinish(testGeneratorThrow));
print("compile testGeneratorReturn: " + ArkTools.waitJitCompileFinish(testGeneratorReturn));
print("compile testDelegatingGenerator: " + ArkTools.waitJitCompileFinish(testDelegatingGenerator));
print("compile testDelegateToArray: " + ArkTools.waitJitCompileFinish(testDelegateToArray));
print("compile testDelegateToString: " + ArkTools.waitJitCompileFinish(testDelegateToString));
print("compile testInfiniteSequence: " + ArkTools.waitJitCompileFinish(testInfiniteSequence));
print("compile testFibonacci: " + ArkTools.waitJitCompileFinish(testFibonacci));
print("compile testGeneratorForOf: " + ArkTools.waitJitCompileFinish(testGeneratorForOf));
print("compile testGeneratorSpread: " + ArkTools.waitJitCompileFinish(testGeneratorSpread));
print("compile testGeneratorDestructuring: " + ArkTools.waitJitCompileFinish(testGeneratorDestructuring));
print("compile testCustomIterable: " + ArkTools.waitJitCompileFinish(testCustomIterable));
print("compile testRangeIterator: " + ArkTools.waitJitCompileFinish(testRangeIterator));
print("compile testTakeHelper: " + ArkTools.waitJitCompileFinish(testTakeHelper));
print("compile testFilterHelper: " + ArkTools.waitJitCompileFinish(testFilterHelper));
print("compile testMapHelper: " + ArkTools.waitJitCompileFinish(testMapHelper));
print("compile testZipHelper: " + ArkTools.waitJitCompileFinish(testZipHelper));
print("compile testChainGenerator: " + ArkTools.waitJitCompileFinish(testChainGenerator));
print("compile testFlattenGenerator: " + ArkTools.waitJitCompileFinish(testFlattenGenerator));
print("compile testStateMachine: " + ArkTools.waitJitCompileFinish(testStateMachine));

// Verify
print("testSimpleGenerator: " + testSimpleGenerator());
print("testGeneratorWithReturn: " + testGeneratorWithReturn());
print("testGeneratorWithParams: " + testGeneratorWithParams());
print("testYieldExpression: " + testYieldExpression());
print("testGeneratorThrow: " + testGeneratorThrow());
print("testGeneratorReturn: " + testGeneratorReturn());
print("testDelegatingGenerator: " + testDelegatingGenerator());
print("testDelegateToArray: " + testDelegateToArray());
print("testDelegateToString: " + testDelegateToString());
print("testInfiniteSequence: " + testInfiniteSequence());
print("testFibonacci: " + testFibonacci());
print("testGeneratorForOf: " + testGeneratorForOf());
print("testGeneratorSpread: " + testGeneratorSpread());
print("testGeneratorDestructuring: " + testGeneratorDestructuring());
print("testCustomIterable: " + testCustomIterable());
print("testRangeIterator: " + testRangeIterator());
print("testTakeHelper: " + testTakeHelper());
print("testFilterHelper: " + testFilterHelper());
print("testMapHelper: " + testMapHelper());
print("testZipHelper: " + testZipHelper());
print("testChainGenerator: " + testChainGenerator());
print("testFlattenGenerator: " + testFlattenGenerator());
print("testStateMachine: " + testStateMachine());
