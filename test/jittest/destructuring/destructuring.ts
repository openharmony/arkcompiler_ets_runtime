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

// ==================== Array Destructuring ====================

// Basic array destructuring
function destructArrayBasic(): string {
    const arr = [1, 2, 3];
    const [a, b, c] = arr;
    return `${a},${b},${c}`;
}

// Nested array destructuring
function destructArrayNested(): string {
    const arr = [[1, 2], [3, 4]];
    const [[a, b], [c, d]] = arr;
    return `${a},${b},${c},${d}`;
}

// Rest element destructuring
function destructArrayRest(): string {
    const arr = [1, 2, 3, 4, 5];
    const [first, second, ...rest] = arr;
    return `${first},${second},${rest.join('-')}`;
}

// Default value destructuring
function destructArrayDefault(): string {
    const arr = [1, 2];
    const [a, b, c = 10, d = 20] = arr;
    return `${a},${b},${c},${d}`;
}

// Skip elements destructuring
function destructArraySkip(): string {
    const arr = [1, 2, 3, 4, 5];
    const [, second, , fourth] = arr;
    return `${second},${fourth}`;
}

// Array destructuring with expressions
function destructArrayExpr(): string {
    const fn = () => [10, 20, 30];
    const [x, y, z] = fn();
    return `${x},${y},${z}`;
}

// Swap variables using destructuring
function destructArraySwap(): string {
    let a = 1, b = 2;
    [a, b] = [b, a];
    return `${a},${b}`;
}

// Destructure string as array
function destructArrayString(): string {
    const str = "hello";
    const [first, second, ...rest] = str;
    return `${first},${second},${rest.length}`;
}

// Array destructuring with undefined elements
function destructArrayUndefined(): string {
    const arr = [1, undefined, 3];
    const [a, b = 99, c] = arr;
    return `${a},${b},${c}`;
}

// Deep nested array destructuring
function destructArrayDeepNested(): string {
    const arr = [1, [2, [3, [4, 5]]]];
    const [a, [b, [c, [d, e]]]] = arr;
    return `${a},${b},${c},${d},${e}`;
}

// Array destructuring with function call
function destructArrayFuncCall(): string {
    function getData(): [number, string, boolean] {
        return [42, 'hello', true];
    }
    const [num, str, bool] = getData();
    return `${num},${str},${bool}`;
}

// Array destructuring with iterator
function destructArrayIterator(): string {
    const set = new Set([1, 2, 3]);
    const [first, second, third] = set;
    return `${first},${second},${third}`;
}

// Empty array destructuring with defaults
function destructArrayEmptyDefaults(): string {
    const arr: number[] = [];
    const [a = 1, b = 2, c = 3] = arr;
    return `${a},${b},${c}`;
}

// Rest with empty array
function destructArrayRestEmpty(): string {
    const arr = [1];
    const [first, ...rest] = arr;
    return `${first},${rest.length}`;
}

// Multiple rest operations
function destructArrayMultiRest(): string {
    const arr1 = [1, 2, 3, 4, 5];
    const [a, ...rest1] = arr1;
    const [b, ...rest2] = rest1;
    return `${a},${b},${rest2.join(',')}`;
}

// ==================== Object Destructuring ====================

// Basic object destructuring
function destructObjectBasic(): string {
    const obj = { x: 10, y: 20 };
    const { x, y } = obj;
    return `${x},${y}`;
}

// Nested object destructuring
function destructObjectNested(): string {
    const obj = { outer: { inner: { value: 42 } } };
    const { outer: { inner: { value } } } = obj;
    return `${value}`;
}

// Rename in destructuring
function destructObjectRename(): string {
    const obj = { name: "Alice", age: 30 };
    const { name: userName, age: userAge } = obj;
    return `${userName},${userAge}`;
}

// Default values in object destructuring
function destructObjectDefault(): string {
    const obj = { a: 1 };
    const { a, b = 100, c = 200 } = obj as any;
    return `${a},${b},${c}`;
}

// Rest in object destructuring
function destructObjectRest(): string {
    const obj = { a: 1, b: 2, c: 3, d: 4 };
    const { a, b, ...rest } = obj;
    return `${a},${b},${Object.keys(rest).length}`;
}

// Computed property names in destructuring
function destructObjectComputed(): string {
    const key = "dynamicKey";
    const obj = { dynamicKey: "dynamicValue", other: 123 };
    const { [key]: value } = obj;
    return `${value}`;
}

// Destructure with existing variables
function destructObjectExisting(): string {
    let x: number, y: number;
    const obj = { x: 100, y: 200 };
    ({ x, y } = obj);
    return `${x},${y}`;
}

// Deep nested destructuring with defaults
function destructObjectDeepDefault(): string {
    const obj = { level1: { level2: {} } } as any;
    const { level1: { level2: { value = "default" } } } = obj;
    return value;
}

// Object destructuring with rename and default
function destructObjectRenameDefault(): string {
    const obj = { name: "Bob" } as any;
    const { name: userName, age: userAge = 25 } = obj;
    return `${userName},${userAge}`;
}

// Destructure prototype properties
function destructObjectPrototype(): string {
    const proto = { inherited: "parent" };
    const obj = Object.create(proto);
    obj.own = "child";
    const { own, inherited } = obj;
    return `${own},${inherited}`;
}

// Multiple computed property destructuring
function destructObjectMultiComputed(): string {
    const keys = ['first', 'second'];
    const obj = { first: 1, second: 2, third: 3 };
    const { [keys[0]]: a, [keys[1]]: b } = obj;
    return `${a},${b}`;
}

// Object destructuring with numeric keys
function destructObjectNumericKeys(): string {
    const obj = { 0: 'zero', 1: 'one', 2: 'two' };
    const { 0: first, 1: second } = obj;
    return `${first},${second}`;
}

// Object destructuring with symbol keys
function destructObjectSymbolKey(): string {
    const sym = Symbol('key');
    const obj = { [sym]: 'symbol value', normal: 'normal value' };
    const { [sym]: symVal, normal } = obj;
    return `${symVal},${normal}`;
}

// Nested object with array
function destructObjectWithArray(): string {
    const obj = { data: { items: [1, 2, 3] } };
    const { data: { items: [first, second, third] } } = obj;
    return `${first},${second},${third}`;
}

// Object destructuring from function return
function destructObjectFromFunc(): string {
    function getUser(): { name: string; score: number } {
        return { name: 'Test', score: 100 };
    }
    const { name, score } = getUser();
    return `${name},${score}`;
}

// ==================== Mixed Destructuring ====================

// Function parameter destructuring - array
function destructParamArray([a, b, c]: number[]): string {
    return `${a + b + c}`;
}

// Function parameter destructuring - object
function destructParamObject({ name, age }: { name: string; age: number }): string {
    return `${name}:${age}`;
}

// Function parameter with defaults
function destructParamDefault({ x = 0, y = 0 }: { x?: number; y?: number } = {}): string {
    return `${x},${y}`;
}

// Mixed array and object destructuring
function destructMixed(): string {
    const data = { items: [1, 2, 3], meta: { count: 3 } };
    const { items: [first, ...rest], meta: { count } } = data;
    return `${first},${rest.length},${count}`;
}

// Destructuring in for-of loop
function destructForOf(): string {
    const pairs = [[1, 'a'], [2, 'b'], [3, 'c']] as [number, string][];
    const result: string[] = [];
    for (const [num, char] of pairs) {
        result.push(`${num}${char}`);
    }
    return result.join(',');
}

// Destructuring Map entries
function destructMapEntries(): string {
    const map = new Map<string, number>([['a', 1], ['b', 2], ['c', 3]]);
    const result: string[] = [];
    for (const [key, value] of map) {
        result.push(`${key}=${value}`);
    }
    return result.join(',');
}

// Destructuring in array methods
function destructArrayMethods(): string {
    const pairs = [[1, 2], [3, 4], [5, 6]];
    const sums = pairs.map(([a, b]) => a + b);
    return sums.join(',');
}

// Destructuring with type coercion
function destructCoercion(): string {
    const arr = [1, '2', true];
    const [num, str, bool] = arr;
    return `${typeof num},${typeof str},${typeof bool}`;
}

// Multiple levels of nesting
function destructMultiLevel(): string {
    const data = {
        users: [
            { name: 'Alice', scores: [90, 85, 88] },
            { name: 'Bob', scores: [78, 82, 80] }
        ]
    };
    const { users: [{ name: firstName, scores: [firstScore] }] } = data;
    return `${firstName}:${firstScore}`;
}

// Destructuring with optional chaining fallback
function destructOptionalFallback(): string {
    const obj: any = { data: { value: 'initial' } };
    const { data: { value = 'fallback' } = {} } = obj;
    return value;
}

// Nested function parameter destructuring
function destructNestedParam({ user: { name, info: { age } } }: { user: { name: string; info: { age: number } } }): string {
    return `${name}:${age}`;
}

// Array destructuring in reduce
function destructInReduce(): string {
    const pairs = [['a', 1], ['b', 2], ['c', 3]] as [string, number][];
    const obj = pairs.reduce((acc, [key, value]) => {
        acc[key] = value;
        return acc;
    }, {} as Record<string, number>);
    return `${obj.a},${obj.b},${obj.c}`;
}

// Destructuring with filter
function destructInFilter(): string {
    const items = [
        { name: 'a', active: true },
        { name: 'b', active: false },
        { name: 'c', active: true }
    ];
    const activeNames = items.filter(({ active }) => active).map(({ name }) => name);
    return activeNames.join(',');
}

// Destructuring with find
function destructInFind(): string {
    const users = [
        { id: 1, name: 'Alice' },
        { id: 2, name: 'Bob' },
        { id: 3, name: 'Charlie' }
    ];
    const { name } = users.find(({ id }) => id === 2) || { name: 'Not found' };
    return name;
}

// Complex nested mixed destructuring
function destructComplexNested(): string {
    const data = {
        response: {
            data: {
                users: [
                    { profile: { name: 'Test', settings: { theme: 'dark' } } }
                ]
            }
        }
    };
    const { response: { data: { users: [{ profile: { name, settings: { theme } } }] } } } = data;
    return `${name},${theme}`;
}

// Destructuring with spread and rest
function destructSpreadRest(): string {
    const arr = [1, 2, 3, 4, 5];
    const [first, ...middle] = arr;
    const [...copy] = middle;
    return `${first},${copy.join(',')}`;
}

// Object destructuring with array spread
function destructObjectArraySpread(): string {
    const obj = { items: [1, 2, 3] };
    const { items: [...spreadItems] } = obj;
    return spreadItems.join(',');
}

// Conditional destructuring
function destructConditional(): string {
    const maybeData: { value: number } | null = Math.random() > -1 ? { value: 42 } : null;
    const { value = 0 } = maybeData || {};
    return `${value}`;
}

// Destructuring in class method
class DestructClass {
    process({ x, y }: { x: number; y: number }): string {
        return `${x + y}`;
    }
    processArray([a, b]: [number, number]): string {
        return `${a * b}`;
    }
}

function destructInClass(): string {
    const instance = new DestructClass();
    const r1 = instance.process({ x: 10, y: 20 });
    const r2 = instance.processArray([3, 4]);
    return `${r1},${r2}`;
}

// Destructuring async result (sync wrapper)
function destructAsyncResult(): string {
    const mockAsyncData = { status: 'success', result: { data: [1, 2, 3] } };
    const { status, result: { data: [first] } } = mockAsyncData;
    return `${status},${first}`;
}

// ==================== Advanced Patterns ====================

// Destructuring with getters
function destructWithGetter(): string {
    const obj = {
        get computed() { return 42; },
        normal: 'value'
    };
    const { computed, normal } = obj;
    return `${computed},${normal}`;
}

// Destructuring JSON-like structure
function destructJSONLike(): string {
    const json = {
        data: {
            attributes: {
                title: 'Test',
                meta: { views: 100 }
            }
        }
    };
    const { data: { attributes: { title, meta: { views } } } } = json;
    return `${title},${views}`;
}

// Destructuring with Object.entries
function destructObjectEntries(): string {
    const obj = { a: 1, b: 2, c: 3 };
    const result: string[] = [];
    for (const [key, value] of Object.entries(obj)) {
        result.push(`${key}:${value}`);
    }
    return result.join(',');
}

// Destructuring with Array.from
function destructArrayFrom(): string {
    const str = 'abc';
    const [first, second, third] = Array.from(str);
    return `${first},${second},${third}`;
}

// Test wrappers for parameterized functions
function testDestructParamArray(): string {
    return destructParamArray([10, 20, 30]);
}

function testDestructParamObject(): string {
    return destructParamObject({ name: 'Test', age: 25 });
}

function testDestructParamDefault(): string {
    const r1 = destructParamDefault({ x: 5 });
    const r2 = destructParamDefault();
    return `${r1}|${r2}`;
}

function testDestructNestedParam(): string {
    return destructNestedParam({ user: { name: 'Alice', info: { age: 30 } } });
}

// Warmup
for (let i = 0; i < 20; i++) {
    destructArrayBasic();
    destructArrayNested();
    destructArrayRest();
    destructArrayDefault();
    destructArraySkip();
    destructArrayExpr();
    destructArraySwap();
    destructArrayString();
    destructArrayUndefined();
    destructArrayDeepNested();
    destructArrayFuncCall();
    destructArrayIterator();
    destructArrayEmptyDefaults();
    destructArrayRestEmpty();
    destructArrayMultiRest();
    destructObjectBasic();
    destructObjectNested();
    destructObjectRename();
    destructObjectDefault();
    destructObjectRest();
    destructObjectComputed();
    destructObjectExisting();
    destructObjectDeepDefault();
    destructObjectRenameDefault();
    destructObjectPrototype();
    destructObjectMultiComputed();
    destructObjectNumericKeys();
    destructObjectSymbolKey();
    destructObjectWithArray();
    destructObjectFromFunc();
    testDestructParamArray();
    testDestructParamObject();
    testDestructParamDefault();
    destructMixed();
    destructForOf();
    destructMapEntries();
    destructArrayMethods();
    destructCoercion();
    destructMultiLevel();
    destructOptionalFallback();
    testDestructNestedParam();
    destructInReduce();
    destructInFilter();
    destructInFind();
    destructComplexNested();
    destructSpreadRest();
    destructObjectArraySpread();
    destructConditional();
    destructInClass();
    destructAsyncResult();
    destructWithGetter();
    destructJSONLike();
    destructObjectEntries();
    destructArrayFrom();
}

// JIT compile
ArkTools.jitCompileAsync(destructArrayBasic);
ArkTools.jitCompileAsync(destructArrayNested);
ArkTools.jitCompileAsync(destructArrayRest);
ArkTools.jitCompileAsync(destructArrayDefault);
ArkTools.jitCompileAsync(destructArraySkip);
ArkTools.jitCompileAsync(destructArrayExpr);
ArkTools.jitCompileAsync(destructArraySwap);
ArkTools.jitCompileAsync(destructArrayString);
ArkTools.jitCompileAsync(destructArrayUndefined);
ArkTools.jitCompileAsync(destructArrayDeepNested);
ArkTools.jitCompileAsync(destructArrayFuncCall);
ArkTools.jitCompileAsync(destructArrayIterator);
ArkTools.jitCompileAsync(destructArrayEmptyDefaults);
ArkTools.jitCompileAsync(destructArrayRestEmpty);
ArkTools.jitCompileAsync(destructArrayMultiRest);
ArkTools.jitCompileAsync(destructObjectBasic);
ArkTools.jitCompileAsync(destructObjectNested);
ArkTools.jitCompileAsync(destructObjectRename);
ArkTools.jitCompileAsync(destructObjectDefault);
ArkTools.jitCompileAsync(destructObjectRest);
ArkTools.jitCompileAsync(destructObjectComputed);
ArkTools.jitCompileAsync(destructObjectExisting);
ArkTools.jitCompileAsync(destructObjectDeepDefault);
ArkTools.jitCompileAsync(destructObjectRenameDefault);
ArkTools.jitCompileAsync(destructObjectPrototype);
ArkTools.jitCompileAsync(destructObjectMultiComputed);
ArkTools.jitCompileAsync(destructObjectNumericKeys);
ArkTools.jitCompileAsync(destructObjectSymbolKey);
ArkTools.jitCompileAsync(destructObjectWithArray);
ArkTools.jitCompileAsync(destructObjectFromFunc);
ArkTools.jitCompileAsync(destructParamArray);
ArkTools.jitCompileAsync(destructParamObject);
ArkTools.jitCompileAsync(destructParamDefault);
ArkTools.jitCompileAsync(destructMixed);
ArkTools.jitCompileAsync(destructForOf);
ArkTools.jitCompileAsync(destructMapEntries);
ArkTools.jitCompileAsync(destructArrayMethods);
ArkTools.jitCompileAsync(destructCoercion);
ArkTools.jitCompileAsync(destructMultiLevel);
ArkTools.jitCompileAsync(destructOptionalFallback);
ArkTools.jitCompileAsync(destructNestedParam);
ArkTools.jitCompileAsync(destructInReduce);
ArkTools.jitCompileAsync(destructInFilter);
ArkTools.jitCompileAsync(destructInFind);
ArkTools.jitCompileAsync(destructComplexNested);
ArkTools.jitCompileAsync(destructSpreadRest);
ArkTools.jitCompileAsync(destructObjectArraySpread);
ArkTools.jitCompileAsync(destructConditional);
ArkTools.jitCompileAsync(destructInClass);
ArkTools.jitCompileAsync(destructAsyncResult);
ArkTools.jitCompileAsync(destructWithGetter);
ArkTools.jitCompileAsync(destructJSONLike);
ArkTools.jitCompileAsync(destructObjectEntries);
ArkTools.jitCompileAsync(destructArrayFrom);

print("compile destructArrayBasic: " + ArkTools.waitJitCompileFinish(destructArrayBasic));
print("compile destructArrayNested: " + ArkTools.waitJitCompileFinish(destructArrayNested));
print("compile destructArrayRest: " + ArkTools.waitJitCompileFinish(destructArrayRest));
print("compile destructArrayDefault: " + ArkTools.waitJitCompileFinish(destructArrayDefault));
print("compile destructArraySkip: " + ArkTools.waitJitCompileFinish(destructArraySkip));
print("compile destructArrayExpr: " + ArkTools.waitJitCompileFinish(destructArrayExpr));
print("compile destructArraySwap: " + ArkTools.waitJitCompileFinish(destructArraySwap));
print("compile destructArrayString: " + ArkTools.waitJitCompileFinish(destructArrayString));
print("compile destructArrayUndefined: " + ArkTools.waitJitCompileFinish(destructArrayUndefined));
print("compile destructArrayDeepNested: " + ArkTools.waitJitCompileFinish(destructArrayDeepNested));
print("compile destructArrayFuncCall: " + ArkTools.waitJitCompileFinish(destructArrayFuncCall));
print("compile destructArrayIterator: " + ArkTools.waitJitCompileFinish(destructArrayIterator));
print("compile destructArrayEmptyDefaults: " + ArkTools.waitJitCompileFinish(destructArrayEmptyDefaults));
print("compile destructArrayRestEmpty: " + ArkTools.waitJitCompileFinish(destructArrayRestEmpty));
print("compile destructArrayMultiRest: " + ArkTools.waitJitCompileFinish(destructArrayMultiRest));
print("compile destructObjectBasic: " + ArkTools.waitJitCompileFinish(destructObjectBasic));
print("compile destructObjectNested: " + ArkTools.waitJitCompileFinish(destructObjectNested));
print("compile destructObjectRename: " + ArkTools.waitJitCompileFinish(destructObjectRename));
print("compile destructObjectDefault: " + ArkTools.waitJitCompileFinish(destructObjectDefault));
print("compile destructObjectRest: " + ArkTools.waitJitCompileFinish(destructObjectRest));
print("compile destructObjectComputed: " + ArkTools.waitJitCompileFinish(destructObjectComputed));
print("compile destructObjectExisting: " + ArkTools.waitJitCompileFinish(destructObjectExisting));
print("compile destructObjectDeepDefault: " + ArkTools.waitJitCompileFinish(destructObjectDeepDefault));
print("compile destructObjectRenameDefault: " + ArkTools.waitJitCompileFinish(destructObjectRenameDefault));
print("compile destructObjectPrototype: " + ArkTools.waitJitCompileFinish(destructObjectPrototype));
print("compile destructObjectMultiComputed: " + ArkTools.waitJitCompileFinish(destructObjectMultiComputed));
print("compile destructObjectNumericKeys: " + ArkTools.waitJitCompileFinish(destructObjectNumericKeys));
print("compile destructObjectSymbolKey: " + ArkTools.waitJitCompileFinish(destructObjectSymbolKey));
print("compile destructObjectWithArray: " + ArkTools.waitJitCompileFinish(destructObjectWithArray));
print("compile destructObjectFromFunc: " + ArkTools.waitJitCompileFinish(destructObjectFromFunc));
print("compile destructParamArray: " + ArkTools.waitJitCompileFinish(destructParamArray));
print("compile destructParamObject: " + ArkTools.waitJitCompileFinish(destructParamObject));
print("compile destructParamDefault: " + ArkTools.waitJitCompileFinish(destructParamDefault));
print("compile destructMixed: " + ArkTools.waitJitCompileFinish(destructMixed));
print("compile destructForOf: " + ArkTools.waitJitCompileFinish(destructForOf));
print("compile destructMapEntries: " + ArkTools.waitJitCompileFinish(destructMapEntries));
print("compile destructArrayMethods: " + ArkTools.waitJitCompileFinish(destructArrayMethods));
print("compile destructCoercion: " + ArkTools.waitJitCompileFinish(destructCoercion));
print("compile destructMultiLevel: " + ArkTools.waitJitCompileFinish(destructMultiLevel));
print("compile destructOptionalFallback: " + ArkTools.waitJitCompileFinish(destructOptionalFallback));
print("compile destructNestedParam: " + ArkTools.waitJitCompileFinish(destructNestedParam));
print("compile destructInReduce: " + ArkTools.waitJitCompileFinish(destructInReduce));
print("compile destructInFilter: " + ArkTools.waitJitCompileFinish(destructInFilter));
print("compile destructInFind: " + ArkTools.waitJitCompileFinish(destructInFind));
print("compile destructComplexNested: " + ArkTools.waitJitCompileFinish(destructComplexNested));
print("compile destructSpreadRest: " + ArkTools.waitJitCompileFinish(destructSpreadRest));
print("compile destructObjectArraySpread: " + ArkTools.waitJitCompileFinish(destructObjectArraySpread));
print("compile destructConditional: " + ArkTools.waitJitCompileFinish(destructConditional));
print("compile destructInClass: " + ArkTools.waitJitCompileFinish(destructInClass));
print("compile destructAsyncResult: " + ArkTools.waitJitCompileFinish(destructAsyncResult));
print("compile destructWithGetter: " + ArkTools.waitJitCompileFinish(destructWithGetter));
print("compile destructJSONLike: " + ArkTools.waitJitCompileFinish(destructJSONLike));
print("compile destructObjectEntries: " + ArkTools.waitJitCompileFinish(destructObjectEntries));
print("compile destructArrayFrom: " + ArkTools.waitJitCompileFinish(destructArrayFrom));

// Verify
print("destructArrayBasic: " + destructArrayBasic());
print("destructArrayNested: " + destructArrayNested());
print("destructArrayRest: " + destructArrayRest());
print("destructArrayDefault: " + destructArrayDefault());
print("destructArraySkip: " + destructArraySkip());
print("destructArrayExpr: " + destructArrayExpr());
print("destructArraySwap: " + destructArraySwap());
print("destructArrayString: " + destructArrayString());
print("destructArrayUndefined: " + destructArrayUndefined());
print("destructArrayDeepNested: " + destructArrayDeepNested());
print("destructArrayFuncCall: " + destructArrayFuncCall());
print("destructArrayIterator: " + destructArrayIterator());
print("destructArrayEmptyDefaults: " + destructArrayEmptyDefaults());
print("destructArrayRestEmpty: " + destructArrayRestEmpty());
print("destructArrayMultiRest: " + destructArrayMultiRest());
print("destructObjectBasic: " + destructObjectBasic());
print("destructObjectNested: " + destructObjectNested());
print("destructObjectRename: " + destructObjectRename());
print("destructObjectDefault: " + destructObjectDefault());
print("destructObjectRest: " + destructObjectRest());
print("destructObjectComputed: " + destructObjectComputed());
print("destructObjectExisting: " + destructObjectExisting());
print("destructObjectDeepDefault: " + destructObjectDeepDefault());
print("destructObjectRenameDefault: " + destructObjectRenameDefault());
print("destructObjectPrototype: " + destructObjectPrototype());
print("destructObjectMultiComputed: " + destructObjectMultiComputed());
print("destructObjectNumericKeys: " + destructObjectNumericKeys());
print("destructObjectSymbolKey: " + destructObjectSymbolKey());
print("destructObjectWithArray: " + destructObjectWithArray());
print("destructObjectFromFunc: " + destructObjectFromFunc());
print("testDestructParamArray: " + testDestructParamArray());
print("testDestructParamObject: " + testDestructParamObject());
print("testDestructParamDefault: " + testDestructParamDefault());
print("destructMixed: " + destructMixed());
print("destructForOf: " + destructForOf());
print("destructMapEntries: " + destructMapEntries());
print("destructArrayMethods: " + destructArrayMethods());
print("destructCoercion: " + destructCoercion());
print("destructMultiLevel: " + destructMultiLevel());
print("destructOptionalFallback: " + destructOptionalFallback());
print("testDestructNestedParam: " + testDestructNestedParam());
print("destructInReduce: " + destructInReduce());
print("destructInFilter: " + destructInFilter());
print("destructInFind: " + destructInFind());
print("destructComplexNested: " + destructComplexNested());
print("destructSpreadRest: " + destructSpreadRest());
print("destructObjectArraySpread: " + destructObjectArraySpread());
print("destructConditional: " + destructConditional());
print("destructInClass: " + destructInClass());
print("destructAsyncResult: " + destructAsyncResult());
print("destructWithGetter: " + destructWithGetter());
print("destructJSONLike: " + destructJSONLike());
print("destructObjectEntries: " + destructObjectEntries());
print("destructArrayFrom: " + destructArrayFrom());
