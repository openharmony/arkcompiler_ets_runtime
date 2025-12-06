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

// ==================== Array Spread ====================

// Basic array spread
function spreadArrayBasic(): string {
    const arr1 = [1, 2, 3];
    const arr2 = [4, 5, 6];
    const merged = [...arr1, ...arr2];
    return merged.join(',');
}

// Array spread with elements
function spreadArrayWithElements(): string {
    const arr = [2, 3, 4];
    const result = [1, ...arr, 5, 6];
    return result.join(',');
}

// Array copy using spread
function spreadArrayCopy(): string {
    const original = [1, 2, 3];
    const copy = [...original];
    copy[0] = 100;
    return `${original[0]},${copy[0]}`;
}

// Spread string into array
function spreadString(): string {
    const str = "hello";
    const chars = [...str];
    return chars.join('-');
}

// Spread in function call
function spreadFunctionCall(): string {
    const args = [1, 2, 3];
    const sum = (a: number, b: number, c: number) => a + b + c;
    return `${sum(...args)}`;
}

// Spread Math.max
function spreadMathMax(): string {
    const numbers = [5, 2, 8, 1, 9, 3];
    const max = Math.max(...numbers);
    const min = Math.min(...numbers);
    return `${max},${min}`;
}

// Spread with Set (deduplication)
function spreadSetDedup(): string {
    const arr = [1, 2, 2, 3, 3, 3, 4];
    const unique = [...new Set(arr)];
    return unique.join(',');
}

// Spread in array literal with expressions
function spreadWithExpressions(): string {
    const getArr = () => [4, 5, 6];
    const result = [1, 2, 3, ...getArr()];
    return result.join(',');
}

// Nested array spread
function spreadNestedArray(): string {
    const nested = [[1, 2], [3, 4]];
    const flat = [...nested[0], ...nested[1]];
    return flat.join(',');
}

// ==================== Object Spread ====================

// Basic object spread
function spreadObjectBasic(): string {
    const obj1 = { a: 1, b: 2 };
    const obj2 = { c: 3, d: 4 };
    const merged = { ...obj1, ...obj2 };
    return `${merged.a},${merged.b},${merged.c},${merged.d}`;
}

// Object spread with override
function spreadObjectOverride(): string {
    const defaults = { x: 0, y: 0, z: 0 };
    const custom = { x: 10, y: 20 };
    const result = { ...defaults, ...custom };
    return `${result.x},${result.y},${result.z}`;
}

// Object spread with new properties
function spreadObjectNewProps(): string {
    const base = { a: 1 };
    const extended = { ...base, b: 2, c: 3 };
    return `${extended.a},${extended.b},${extended.c}`;
}

// Object copy using spread
function spreadObjectCopy(): string {
    const original = { x: 10, y: 20 };
    const copy = { ...original };
    copy.x = 100;
    return `${original.x},${copy.x}`;
}

// Nested object spread (shallow)
function spreadObjectNested(): string {
    const obj = { a: 1, nested: { b: 2 } };
    const copy = { ...obj };
    copy.nested.b = 200;
    return `${obj.nested.b},${copy.nested.b}`;
}

// Spread with computed properties
function spreadObjectComputed(): string {
    const key = 'dynamic';
    const base = { a: 1 };
    const result = { ...base, [key]: 'value' };
    return `${result.a},${(result as any)[key]}`;
}

// Spread order matters
function spreadObjectOrder(): string {
    const result = { a: 1, ...{ a: 2 }, ...{ a: 3 } };
    return `${result.a}`;
}

// ==================== Rest Parameters ====================

// Basic rest parameters
function restBasic(...args: number[]): string {
    return args.join(',');
}

// Rest with leading params
function restWithLeading(first: number, second: number, ...rest: number[]): string {
    return `${first},${second},${rest.join('-')}`;
}

// Rest parameter length
function restLength(...args: any[]): string {
    return `${args.length}`;
}

// Rest with different types
function restMixedTypes(...args: (number | string)[]): string {
    return args.map(a => typeof a).join(',');
}

// Rest in arrow function
const restArrow = (...nums: number[]): string => {
    return nums.reduce((a, b) => a + b, 0).toString();
};

// Rest with destructuring
function restWithDestructure([first, ...rest]: number[]): string {
    return `${first}:${rest.join(',')}`;
}

// Object rest in destructuring
function restObjectDestructure(): string {
    const obj = { a: 1, b: 2, c: 3, d: 4 };
    const { a, ...rest } = obj;
    return `${a},${Object.keys(rest).length}`;
}

// Nested rest
function restNested(...arrays: number[][]): string {
    const flattened = arrays.flat();
    return flattened.join(',');
}

// ==================== Combined Patterns ====================

// Spread and rest together
function spreadRestCombined(): string {
    const collectAndSpread = (...args: number[]) => [...args, ...args];
    const result = collectAndSpread(1, 2, 3);
    return result.join(',');
}

// Clone and extend array
function cloneExtendArray(): string {
    const original = [1, 2, 3];
    const extended = [...original, 4, 5];
    return `${original.length},${extended.length}`;
}

// Clone and extend object
function cloneExtendObject(): string {
    const original = { a: 1 };
    const extended = { ...original, b: 2 };
    return `${Object.keys(original).length},${Object.keys(extended).length}`;
}

// Spread in class constructor
class SpreadConstructor {
    values: number[];
    constructor(...args: number[]) {
        this.values = [...args];
    }
    getValues(): string {
        return this.values.join(',');
    }
}

function testSpreadConstructor(): string {
    const obj = new SpreadConstructor(1, 2, 3, 4, 5);
    return obj.getValues();
}

// Spread in method call
function spreadMethodCall(): string {
    const arr = [1, 2, 3, 4, 5];
    const result = arr.concat(...[[6, 7], [8, 9]]);
    return result.join(',');
}

// Rest in callback
function restCallback(): string {
    const process = (callback: (...args: number[]) => number) => {
        return callback(1, 2, 3, 4, 5);
    };
    const sum = (...nums: number[]) => nums.reduce((a, b) => a + b, 0);
    return `${process(sum)}`;
}

// Test wrappers
function testRestBasic(): string {
    return restBasic(1, 2, 3, 4, 5);
}

function testRestWithLeading(): string {
    return restWithLeading(1, 2, 3, 4, 5);
}

function testRestLength(): string {
    return restLength(1, 'a', true, null, undefined);
}

function testRestMixedTypes(): string {
    return restMixedTypes(1, 'a', 2, 'b');
}

function testRestArrow(): string {
    return restArrow(1, 2, 3, 4, 5);
}

function testRestWithDestructure(): string {
    return restWithDestructure([1, 2, 3, 4, 5]);
}

function testRestNested(): string {
    return restNested([1, 2], [3, 4], [5, 6]);
}

// Warmup
for (let i = 0; i < 20; i++) {
    spreadArrayBasic();
    spreadArrayWithElements();
    spreadArrayCopy();
    spreadString();
    spreadFunctionCall();
    spreadMathMax();
    spreadSetDedup();
    spreadWithExpressions();
    spreadNestedArray();
    spreadObjectBasic();
    spreadObjectOverride();
    spreadObjectNewProps();
    spreadObjectCopy();
    spreadObjectNested();
    spreadObjectComputed();
    spreadObjectOrder();
    testRestBasic();
    testRestWithLeading();
    testRestLength();
    testRestMixedTypes();
    testRestArrow();
    testRestWithDestructure();
    restObjectDestructure();
    testRestNested();
    spreadRestCombined();
    cloneExtendArray();
    cloneExtendObject();
    testSpreadConstructor();
    spreadMethodCall();
    restCallback();
}

// JIT compile
ArkTools.jitCompileAsync(spreadArrayBasic);
ArkTools.jitCompileAsync(spreadArrayWithElements);
ArkTools.jitCompileAsync(spreadArrayCopy);
ArkTools.jitCompileAsync(spreadString);
ArkTools.jitCompileAsync(spreadFunctionCall);
ArkTools.jitCompileAsync(spreadMathMax);
ArkTools.jitCompileAsync(spreadSetDedup);
ArkTools.jitCompileAsync(spreadWithExpressions);
ArkTools.jitCompileAsync(spreadNestedArray);
ArkTools.jitCompileAsync(spreadObjectBasic);
ArkTools.jitCompileAsync(spreadObjectOverride);
ArkTools.jitCompileAsync(spreadObjectNewProps);
ArkTools.jitCompileAsync(spreadObjectCopy);
ArkTools.jitCompileAsync(spreadObjectNested);
ArkTools.jitCompileAsync(spreadObjectComputed);
ArkTools.jitCompileAsync(spreadObjectOrder);
ArkTools.jitCompileAsync(restBasic);
ArkTools.jitCompileAsync(restWithLeading);
ArkTools.jitCompileAsync(restLength);
ArkTools.jitCompileAsync(restMixedTypes);
ArkTools.jitCompileAsync(restWithDestructure);
ArkTools.jitCompileAsync(restObjectDestructure);
ArkTools.jitCompileAsync(restNested);
ArkTools.jitCompileAsync(spreadRestCombined);
ArkTools.jitCompileAsync(cloneExtendArray);
ArkTools.jitCompileAsync(cloneExtendObject);
ArkTools.jitCompileAsync(testSpreadConstructor);
ArkTools.jitCompileAsync(spreadMethodCall);
ArkTools.jitCompileAsync(restCallback);

print("compile spreadArrayBasic: " + ArkTools.waitJitCompileFinish(spreadArrayBasic));
print("compile spreadArrayWithElements: " + ArkTools.waitJitCompileFinish(spreadArrayWithElements));
print("compile spreadArrayCopy: " + ArkTools.waitJitCompileFinish(spreadArrayCopy));
print("compile spreadString: " + ArkTools.waitJitCompileFinish(spreadString));
print("compile spreadFunctionCall: " + ArkTools.waitJitCompileFinish(spreadFunctionCall));
print("compile spreadMathMax: " + ArkTools.waitJitCompileFinish(spreadMathMax));
print("compile spreadSetDedup: " + ArkTools.waitJitCompileFinish(spreadSetDedup));
print("compile spreadWithExpressions: " + ArkTools.waitJitCompileFinish(spreadWithExpressions));
print("compile spreadNestedArray: " + ArkTools.waitJitCompileFinish(spreadNestedArray));
print("compile spreadObjectBasic: " + ArkTools.waitJitCompileFinish(spreadObjectBasic));
print("compile spreadObjectOverride: " + ArkTools.waitJitCompileFinish(spreadObjectOverride));
print("compile spreadObjectNewProps: " + ArkTools.waitJitCompileFinish(spreadObjectNewProps));
print("compile spreadObjectCopy: " + ArkTools.waitJitCompileFinish(spreadObjectCopy));
print("compile spreadObjectNested: " + ArkTools.waitJitCompileFinish(spreadObjectNested));
print("compile spreadObjectComputed: " + ArkTools.waitJitCompileFinish(spreadObjectComputed));
print("compile spreadObjectOrder: " + ArkTools.waitJitCompileFinish(spreadObjectOrder));
print("compile restBasic: " + ArkTools.waitJitCompileFinish(restBasic));
print("compile restWithLeading: " + ArkTools.waitJitCompileFinish(restWithLeading));
print("compile restLength: " + ArkTools.waitJitCompileFinish(restLength));
print("compile restMixedTypes: " + ArkTools.waitJitCompileFinish(restMixedTypes));
print("compile restWithDestructure: " + ArkTools.waitJitCompileFinish(restWithDestructure));
print("compile restObjectDestructure: " + ArkTools.waitJitCompileFinish(restObjectDestructure));
print("compile restNested: " + ArkTools.waitJitCompileFinish(restNested));
print("compile spreadRestCombined: " + ArkTools.waitJitCompileFinish(spreadRestCombined));
print("compile cloneExtendArray: " + ArkTools.waitJitCompileFinish(cloneExtendArray));
print("compile cloneExtendObject: " + ArkTools.waitJitCompileFinish(cloneExtendObject));
print("compile testSpreadConstructor: " + ArkTools.waitJitCompileFinish(testSpreadConstructor));
print("compile spreadMethodCall: " + ArkTools.waitJitCompileFinish(spreadMethodCall));
print("compile restCallback: " + ArkTools.waitJitCompileFinish(restCallback));

// Verify
print("spreadArrayBasic: " + spreadArrayBasic());
print("spreadArrayWithElements: " + spreadArrayWithElements());
print("spreadArrayCopy: " + spreadArrayCopy());
print("spreadString: " + spreadString());
print("spreadFunctionCall: " + spreadFunctionCall());
print("spreadMathMax: " + spreadMathMax());
print("spreadSetDedup: " + spreadSetDedup());
print("spreadWithExpressions: " + spreadWithExpressions());
print("spreadNestedArray: " + spreadNestedArray());
print("spreadObjectBasic: " + spreadObjectBasic());
print("spreadObjectOverride: " + spreadObjectOverride());
print("spreadObjectNewProps: " + spreadObjectNewProps());
print("spreadObjectCopy: " + spreadObjectCopy());
print("spreadObjectNested: " + spreadObjectNested());
print("spreadObjectComputed: " + spreadObjectComputed());
print("spreadObjectOrder: " + spreadObjectOrder());
print("testRestBasic: " + testRestBasic());
print("testRestWithLeading: " + testRestWithLeading());
print("testRestLength: " + testRestLength());
print("testRestMixedTypes: " + testRestMixedTypes());
print("testRestArrow: " + testRestArrow());
print("testRestWithDestructure: " + testRestWithDestructure());
print("restObjectDestructure: " + restObjectDestructure());
print("testRestNested: " + testRestNested());
print("spreadRestCombined: " + spreadRestCombined());
print("cloneExtendArray: " + cloneExtendArray());
print("cloneExtendObject: " + cloneExtendObject());
print("testSpreadConstructor: " + testSpreadConstructor());
print("spreadMethodCall: " + spreadMethodCall());
print("restCallback: " + restCallback());
