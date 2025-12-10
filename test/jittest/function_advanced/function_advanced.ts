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

// ==================== Function Properties ====================

function testFunctionName(): string {
    function namedFunc() {}
    const arrowFunc = () => {};
    const obj = { method() {} };
    return `${namedFunc.name},${arrowFunc.name},${obj.method.name}`;
}

function testFunctionLength(): string {
    function f0() {}
    function f1(a: number) { return a; }
    function f2(a: number, b: number) { return a + b; }
    function f3(a: number, b: number, c: number = 0) { return a + b + c; }
    return `${f0.length},${f1.length},${f2.length},${f3.length}`;
}

function testFunctionToString(): string {
    function add(a: number, b: number) { return a + b; }
    const str = add.toString();
    return `${str.length > 0},${typeof str === 'string'}`;
}

// ==================== Call and Apply ====================

function testFunctionCall(): string {
    function greet(this: { name: string }, greeting: string): string {
        return `${greeting}, ${this.name}`;
    }
    const obj = { name: 'Alice' };
    return greet.call(obj, 'Hello');
}

function testFunctionApply(): string {
    function sum(a: number, b: number, c: number): number {
        return a + b + c;
    }
    return `${sum.apply(null, [1, 2, 3])}`;
}

function testFunctionCallWithThis(): string {
    const obj = {
        value: 10,
        getValue(this: { value: number }): number {
            return this.value;
        }
    };
    const other = { value: 20 };
    return `${obj.getValue.call(other)}`;
}

function testFunctionApplyArray(): string {
    const arr = [5, 2, 8, 1, 9];
    const max = Math.max.apply(null, arr);
    const min = Math.min.apply(null, arr);
    return `${max},${min}`;
}

// ==================== Bind ====================

function testFunctionBind(): string {
    function greet(this: { name: string }): string {
        return `Hello, ${this.name}`;
    }
    const obj = { name: 'Bob' };
    const boundGreet = greet.bind(obj);
    return boundGreet();
}

function testFunctionBindWithArgs(): string {
    function add(a: number, b: number, c: number): number {
        return a + b + c;
    }
    const add5 = add.bind(null, 5);
    const add5and10 = add.bind(null, 5, 10);
    return `${add5(10, 20)},${add5and10(20)}`;
}

function testFunctionBindChained(): string {
    function multiply(a: number, b: number, c: number): number {
        return a * b * c;
    }
    const mult2 = multiply.bind(null, 2);
    const mult2by3 = mult2.bind(null, 3);
    return `${mult2by3(4)}`;
}

// ==================== Closures ====================

function testClosureBasic(): string {
    function outer(x: number) {
        return function inner(y: number): number {
            return x + y;
        };
    }
    const add10 = outer(10);
    return `${add10(5)},${add10(20)}`;
}

function testClosureCounter(): string {
    function createCounter() {
        let count = 0;
        return {
            increment(): number { return ++count; },
            decrement(): number { return --count; },
            getCount(): number { return count; }
        };
    }
    const counter = createCounter();
    counter.increment();
    counter.increment();
    counter.increment();
    counter.decrement();
    return `${counter.getCount()}`;
}

function testClosurePrivateState(): string {
    function createPerson(name: string) {
        let _name = name;
        return {
            getName(): string { return _name; },
            setName(newName: string): void { _name = newName; }
        };
    }
    const person = createPerson('Alice');
    const initialName = person.getName();
    person.setName('Bob');
    return `${initialName},${person.getName()}`;
}

function testClosureLoop(): string {
    const funcs: (() => number)[] = [];
    for (let i = 0; i < 3; i++) {
        funcs.push(() => i);
    }
    return funcs.map(f => f()).join(',');
}

// ==================== Higher-Order Functions ====================

function testHigherOrderMap(): string {
    function mapArray<T, U>(arr: T[], fn: (item: T) => U): U[] {
        const result: U[] = [];
        for (const item of arr) {
            result.push(fn(item));
        }
        return result;
    }
    const doubled = mapArray([1, 2, 3], x => x * 2);
    return doubled.join(',');
}

function testHigherOrderFilter(): string {
    function filterArray<T>(arr: T[], predicate: (item: T) => boolean): T[] {
        const result: T[] = [];
        for (const item of arr) {
            if (predicate(item)) result.push(item);
        }
        return result;
    }
    const evens = filterArray([1, 2, 3, 4, 5, 6], x => x % 2 === 0);
    return evens.join(',');
}

function testHigherOrderReduce(): string {
    function reduceArray<T, U>(arr: T[], fn: (acc: U, item: T) => U, initial: U): U {
        let acc = initial;
        for (const item of arr) {
            acc = fn(acc, item);
        }
        return acc;
    }
    const sum = reduceArray([1, 2, 3, 4], (acc, x) => acc + x, 0);
    return `${sum}`;
}

function testCompose(): string {
    function compose<T>(...fns: ((x: T) => T)[]): (x: T) => T {
        return (x: T) => fns.reduceRight((acc, fn) => fn(acc), x);
    }
    const add1 = (x: number) => x + 1;
    const mult2 = (x: number) => x * 2;
    const sub3 = (x: number) => x - 3;
    const composed = compose(add1, mult2, sub3);
    return `${composed(5)}`;
}

function testPipe(): string {
    function pipe<T>(...fns: ((x: T) => T)[]): (x: T) => T {
        return (x: T) => fns.reduce((acc, fn) => fn(acc), x);
    }
    const add1 = (x: number) => x + 1;
    const mult2 = (x: number) => x * 2;
    const sub3 = (x: number) => x - 3;
    const piped = pipe(add1, mult2, sub3);
    return `${piped(5)}`;
}

// ==================== Currying ====================

function testCurryManual(): string {
    function curry(fn: (a: number, b: number, c: number) => number) {
        return (a: number) => (b: number) => (c: number) => fn(a, b, c);
    }
    function add(a: number, b: number, c: number): number {
        return a + b + c;
    }
    const curriedAdd = curry(add);
    return `${curriedAdd(1)(2)(3)}`;
}

function testPartialApplication(): string {
    function partial<T, U, V>(fn: (a: T, b: U) => V, a: T): (b: U) => V {
        return (b: U) => fn(a, b);
    }
    function multiply(a: number, b: number): number {
        return a * b;
    }
    const double = partial(multiply, 2);
    const triple = partial(multiply, 3);
    return `${double(5)},${triple(5)}`;
}

// ==================== Memoization ====================

function testMemoization(): string {
    function memoize<T extends (...args: any[]) => any>(fn: T): T {
        const cache = new Map<string, any>();
        return ((...args: any[]) => {
            const key = JSON.stringify(args);
            if (cache.has(key)) {
                return cache.get(key);
            }
            const result = fn(...args);
            cache.set(key, result);
            return result;
        }) as T;
    }
    let callCount = 0;
    const expensive = memoize((n: number) => {
        callCount++;
        return n * 2;
    });
    expensive(5);
    expensive(5);
    expensive(10);
    expensive(5);
    return `${callCount}`;
}

// ==================== Recursion ====================

function testRecursiveFactorial(): string {
    function factorial(n: number): number {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }
    return `${factorial(5)},${factorial(0)},${factorial(1)}`;
}

function testRecursiveFibonacci(): string {
    function fib(n: number): number {
        if (n <= 1) return n;
        return fib(n - 1) + fib(n - 2);
    }
    return `${fib(0)},${fib(1)},${fib(5)},${fib(10)}`;
}

function testTailRecursion(): string {
    function factorialTail(n: number, acc: number = 1): number {
        if (n <= 1) return acc;
        return factorialTail(n - 1, n * acc);
    }
    return `${factorialTail(5)}`;
}

// ==================== IIFE ====================

function testIIFE(): string {
    const result = ((x: number) => x * 2)(5);
    return `${result}`;
}

function testIIFEWithState(): string {
    const counter = (() => {
        let count = 0;
        return {
            inc(): number { return ++count; },
            get(): number { return count; }
        };
    })();
    counter.inc();
    counter.inc();
    return `${counter.get()}`;
}

// ==================== Arrow Functions ====================

function testArrowThis(): string {
    const obj = {
        value: 42,
        getValueArrow: function() {
            const arrow = () => this.value;
            return arrow();
        }
    };
    return `${obj.getValueArrow()}`;
}

function testArrowNoArguments(): string {
    function regular(...args: number[]): number[] {
        const arrow = () => args;
        return arrow();
    }
    return regular(1, 2, 3).join(',');
}

// ==================== Default Parameters ====================

function testDefaultParams(): string {
    function greet(name: string = 'World', greeting: string = 'Hello'): string {
        return `${greeting}, ${name}`;
    }
    return `${greet()},${greet('Alice')},${greet('Bob', 'Hi')}`;
}

function testDefaultParamsExpression(): string {
    function createArray(size: number = 3, fill: number = size * 2): number[] {
        return new Array(size).fill(fill);
    }
    return `${createArray().join(',')},${createArray(2).join(',')}`;
}

// ==================== Rest Parameters ====================

function testRestParams(): string {
    function sum(...numbers: number[]): number {
        return numbers.reduce((a, b) => a + b, 0);
    }
    return `${sum(1, 2, 3)},${sum(1, 2, 3, 4, 5)}`;
}

function testRestWithRequired(): string {
    function format(prefix: string, ...values: number[]): string {
        return `${prefix}: ${values.join(', ')}`;
    }
    return format('Numbers', 1, 2, 3);
}

// ==================== Generator Functions ====================

function testGeneratorBasic(): string {
    function* gen(): Generator<number> {
        yield 1;
        yield 2;
        yield 3;
    }
    return [...gen()].join(',');
}

function testGeneratorWithReturn(): string {
    function* gen(): Generator<number, string> {
        yield 1;
        yield 2;
        return 'done';
    }
    const g = gen();
    const results: string[] = [];
    let result = g.next();
    while (!result.done) {
        results.push(String(result.value));
        result = g.next();
    }
    results.push(result.value);
    return results.join(',');
}

// ==================== Function Constructor ====================

function testFunctionAsObject(): string {
    function fn() {}
    fn.customProp = 'value';
    (fn as any).counter = 0;
    return `${(fn as any).customProp},${(fn as any).counter}`;
}

// Warmup
for (let i = 0; i < 20; i++) {
    testFunctionName();
    testFunctionLength();
    testFunctionToString();
    testFunctionCall();
    testFunctionApply();
    testFunctionCallWithThis();
    testFunctionApplyArray();
    testFunctionBind();
    testFunctionBindWithArgs();
    testFunctionBindChained();
    testClosureBasic();
    testClosureCounter();
    testClosurePrivateState();
    testClosureLoop();
    testHigherOrderMap();
    testHigherOrderFilter();
    testHigherOrderReduce();
    testCompose();
    testPipe();
    testCurryManual();
    testPartialApplication();
    testMemoization();
    testRecursiveFactorial();
    testRecursiveFibonacci();
    testTailRecursion();
    testIIFE();
    testIIFEWithState();
    testArrowThis();
    testArrowNoArguments();
    testDefaultParams();
    testDefaultParamsExpression();
    testRestParams();
    testRestWithRequired();
    testGeneratorBasic();
    testGeneratorWithReturn();
    testFunctionAsObject();
}

// JIT compile
ArkTools.jitCompileAsync(testFunctionName);
ArkTools.jitCompileAsync(testFunctionLength);
ArkTools.jitCompileAsync(testFunctionToString);
ArkTools.jitCompileAsync(testFunctionCall);
ArkTools.jitCompileAsync(testFunctionApply);
ArkTools.jitCompileAsync(testFunctionCallWithThis);
ArkTools.jitCompileAsync(testFunctionApplyArray);
ArkTools.jitCompileAsync(testFunctionBind);
ArkTools.jitCompileAsync(testFunctionBindWithArgs);
ArkTools.jitCompileAsync(testFunctionBindChained);
ArkTools.jitCompileAsync(testClosureBasic);
ArkTools.jitCompileAsync(testClosureCounter);
ArkTools.jitCompileAsync(testClosurePrivateState);
ArkTools.jitCompileAsync(testClosureLoop);
ArkTools.jitCompileAsync(testHigherOrderMap);
ArkTools.jitCompileAsync(testHigherOrderFilter);
ArkTools.jitCompileAsync(testHigherOrderReduce);
ArkTools.jitCompileAsync(testCompose);
ArkTools.jitCompileAsync(testPipe);
ArkTools.jitCompileAsync(testCurryManual);
ArkTools.jitCompileAsync(testPartialApplication);
ArkTools.jitCompileAsync(testMemoization);
ArkTools.jitCompileAsync(testRecursiveFactorial);
ArkTools.jitCompileAsync(testRecursiveFibonacci);
ArkTools.jitCompileAsync(testTailRecursion);
ArkTools.jitCompileAsync(testIIFE);
ArkTools.jitCompileAsync(testIIFEWithState);
ArkTools.jitCompileAsync(testArrowThis);
ArkTools.jitCompileAsync(testArrowNoArguments);
ArkTools.jitCompileAsync(testDefaultParams);
ArkTools.jitCompileAsync(testDefaultParamsExpression);
ArkTools.jitCompileAsync(testRestParams);
ArkTools.jitCompileAsync(testRestWithRequired);
ArkTools.jitCompileAsync(testGeneratorBasic);
ArkTools.jitCompileAsync(testGeneratorWithReturn);
ArkTools.jitCompileAsync(testFunctionAsObject);

print("compile testFunctionName: " + ArkTools.waitJitCompileFinish(testFunctionName));
print("compile testFunctionLength: " + ArkTools.waitJitCompileFinish(testFunctionLength));
print("compile testFunctionToString: " + ArkTools.waitJitCompileFinish(testFunctionToString));
print("compile testFunctionCall: " + ArkTools.waitJitCompileFinish(testFunctionCall));
print("compile testFunctionApply: " + ArkTools.waitJitCompileFinish(testFunctionApply));
print("compile testFunctionCallWithThis: " + ArkTools.waitJitCompileFinish(testFunctionCallWithThis));
print("compile testFunctionApplyArray: " + ArkTools.waitJitCompileFinish(testFunctionApplyArray));
print("compile testFunctionBind: " + ArkTools.waitJitCompileFinish(testFunctionBind));
print("compile testFunctionBindWithArgs: " + ArkTools.waitJitCompileFinish(testFunctionBindWithArgs));
print("compile testFunctionBindChained: " + ArkTools.waitJitCompileFinish(testFunctionBindChained));
print("compile testClosureBasic: " + ArkTools.waitJitCompileFinish(testClosureBasic));
print("compile testClosureCounter: " + ArkTools.waitJitCompileFinish(testClosureCounter));
print("compile testClosurePrivateState: " + ArkTools.waitJitCompileFinish(testClosurePrivateState));
print("compile testClosureLoop: " + ArkTools.waitJitCompileFinish(testClosureLoop));
print("compile testHigherOrderMap: " + ArkTools.waitJitCompileFinish(testHigherOrderMap));
print("compile testHigherOrderFilter: " + ArkTools.waitJitCompileFinish(testHigherOrderFilter));
print("compile testHigherOrderReduce: " + ArkTools.waitJitCompileFinish(testHigherOrderReduce));
print("compile testCompose: " + ArkTools.waitJitCompileFinish(testCompose));
print("compile testPipe: " + ArkTools.waitJitCompileFinish(testPipe));
print("compile testCurryManual: " + ArkTools.waitJitCompileFinish(testCurryManual));
print("compile testPartialApplication: " + ArkTools.waitJitCompileFinish(testPartialApplication));
print("compile testMemoization: " + ArkTools.waitJitCompileFinish(testMemoization));
print("compile testRecursiveFactorial: " + ArkTools.waitJitCompileFinish(testRecursiveFactorial));
print("compile testRecursiveFibonacci: " + ArkTools.waitJitCompileFinish(testRecursiveFibonacci));
print("compile testTailRecursion: " + ArkTools.waitJitCompileFinish(testTailRecursion));
print("compile testIIFE: " + ArkTools.waitJitCompileFinish(testIIFE));
print("compile testIIFEWithState: " + ArkTools.waitJitCompileFinish(testIIFEWithState));
print("compile testArrowThis: " + ArkTools.waitJitCompileFinish(testArrowThis));
print("compile testArrowNoArguments: " + ArkTools.waitJitCompileFinish(testArrowNoArguments));
print("compile testDefaultParams: " + ArkTools.waitJitCompileFinish(testDefaultParams));
print("compile testDefaultParamsExpression: " + ArkTools.waitJitCompileFinish(testDefaultParamsExpression));
print("compile testRestParams: " + ArkTools.waitJitCompileFinish(testRestParams));
print("compile testRestWithRequired: " + ArkTools.waitJitCompileFinish(testRestWithRequired));
print("compile testGeneratorBasic: " + ArkTools.waitJitCompileFinish(testGeneratorBasic));
print("compile testGeneratorWithReturn: " + ArkTools.waitJitCompileFinish(testGeneratorWithReturn));
print("compile testFunctionAsObject: " + ArkTools.waitJitCompileFinish(testFunctionAsObject));

// Verify
print("testFunctionName: " + testFunctionName());
print("testFunctionLength: " + testFunctionLength());
print("testFunctionToString: " + testFunctionToString());
print("testFunctionCall: " + testFunctionCall());
print("testFunctionApply: " + testFunctionApply());
print("testFunctionCallWithThis: " + testFunctionCallWithThis());
print("testFunctionApplyArray: " + testFunctionApplyArray());
print("testFunctionBind: " + testFunctionBind());
print("testFunctionBindWithArgs: " + testFunctionBindWithArgs());
print("testFunctionBindChained: " + testFunctionBindChained());
print("testClosureBasic: " + testClosureBasic());
print("testClosureCounter: " + testClosureCounter());
print("testClosurePrivateState: " + testClosurePrivateState());
print("testClosureLoop: " + testClosureLoop());
print("testHigherOrderMap: " + testHigherOrderMap());
print("testHigherOrderFilter: " + testHigherOrderFilter());
print("testHigherOrderReduce: " + testHigherOrderReduce());
print("testCompose: " + testCompose());
print("testPipe: " + testPipe());
print("testCurryManual: " + testCurryManual());
print("testPartialApplication: " + testPartialApplication());
print("testMemoization: " + testMemoization());
print("testRecursiveFactorial: " + testRecursiveFactorial());
print("testRecursiveFibonacci: " + testRecursiveFibonacci());
print("testTailRecursion: " + testTailRecursion());
print("testIIFE: " + testIIFE());
print("testIIFEWithState: " + testIIFEWithState());
print("testArrowThis: " + testArrowThis());
print("testArrowNoArguments: " + testArrowNoArguments());
print("testDefaultParams: " + testDefaultParams());
print("testDefaultParamsExpression: " + testDefaultParamsExpression());
print("testRestParams: " + testRestParams());
print("testRestWithRequired: " + testRestWithRequired());
print("testGeneratorBasic: " + testGeneratorBasic());
print("testGeneratorWithReturn: " + testGeneratorWithReturn());
print("testFunctionAsObject: " + testFunctionAsObject());
