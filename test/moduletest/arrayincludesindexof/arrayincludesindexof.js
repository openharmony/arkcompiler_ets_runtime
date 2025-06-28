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

print('======== Testing fromIndex ========');
{
    const arr = [-10, 42, 42, 42, 42, -20];
    const numberIndices = [
          4,  4.1,  4.5,  4.9,                      // All truncated to  4
         -0, -0.1, -0.5, -0.9, -1 + Number.EPSILON, // All truncated to  0
         -1, -1.1, -1.5, -1.9, -2 + Number.EPSILON, // All truncated to -1
        1/0, -1/0,  0/0,  NaN, // +inf, -inf, NaN, NaN
    ];
    const objN = { valueOf() { return 2; } };
    const objS = { valueOf() { return "3.456" } };
    const objB = { valueOf() { return 4n; } };
    const objNN = { valueOf() { return objN; } };
    const nonNumberIndices = [
        null, undefined,
        "1", "1.234", "2.0?", "~3.0~",
        new Number(2),
        objN,
        new String("3.123"),
        objS,
        objNN,
    ];
    const erroneousIndices = [
        4n, objB,
    ];

    for (const fnName of ['indexOf', 'lastIndexOf', 'includes']) {
        print(`---- Testing Array.prototype.${fnName} ----`);
        const fn = Array.prototype[fnName];
        print(`Result with numberIndices: ${numberIndices.map((i) => fn.call(arr, 42, i))}`);
        print(`Result with nonNumberIndices: ${nonNumberIndices.map((i) => fn.call(arr, 42, i))}`);
        for (const i of erroneousIndices) {
            try {
                const result = fn.call(arr, 42, i);
                print(`Unexpected result: ${result} (Expects: TypeError)`);
            } catch (e) {
                if (e instanceof TypeError) {
                    print(`OK: TypeError raised for ${typeof i} index`);
                } else {
                    print(`ERROR: Unexpected type of exception raised`);
                    throw e;
                }
            }
        }
    }
}

print('======== Testing comparision ========');
{
    const obj = { x: 1, y: 2 };
    const objN = { valueOf() { return 0; } };
    const arr = [
        1, 1.0, -1.0, // 1
        0, 0.0, -0.0, // 0
        1/0, -1/0,    // inf
        0/0,  NaN,    // NaN
        true, false, null,      // Special values
        /* hole */,
        "1", "0", "1.0", "0.0", // string
        1n, 0n,                 // bigint
        obj, objN,              // object
        new Number(0),          // Boxing number
        new String("1"),        // Boxing string
    ];
    const targets = [
        1, 1.0, -1.0, // 1
        0, 0.0, -0.0, // 0
        1/0, -1/0,    // inf
        0/0,  NaN,    // NaN
        true, false, null, undefined, // Special values
        "1", "0", "1.0", "0.0", // string
        1n, 0n,                 // bigint
        obj, objN,              // object
        Math.sqrt(-1),  // Another NaN
        { x: 1, y: 2 }, // Another object
        { valueOf() { return 0; } }, // Another object
        new Number(0),      // Boxing number
        new String("1"),    // Boxing string
    ];
    for (let i = 0; i < targets.length; i++) {
        const x = targets[i];
        const res1 = arr.indexOf(x);
        const res2 = arr.lastIndexOf(x);
        const res3 = arr.includes(x);
        print(`[${i}] Target = ${x} (${typeof x}): indexOf => ${res1} lastIndexOf => ${res2} includes => ${res3}`);
    }
    // Fills the hole with undefined
    arr[12] = undefined;
    {
        const res1 = arr.indexOf(undefined);
        const res2 = arr.lastIndexOf(undefined);
        const res3 = arr.includes(undefined);
        print(`Index = undefined: indexOf => ${res1} lastIndexOf => ${res2} includes => ${res3}`);
    }
}

let makeElementSeed = 0;
function makeElement(type) {
    makeElementSeed += 1;
    if (type === 'int') {
        return (makeElementSeed * 5 + 1) % 11;
    } else if (type === 'double') {
        return (makeElementSeed * 0.625 + 1) % 1.375;
    } else if (type === 'string') {
        return ((makeElementSeed * 5 + 1) % 11).toString();
    } else if (type === 'bigint') {
        return BigInt((makeElementSeed * 5 + 1) % 11);
    } else {
        return { value: (makeElementSeed * 5 + 1) % 11 };
    }
}

print('======== Testing with non-generic ElementsKind ========');
{
    const intArr = [];
    for (let i = 0; i < 11; i++) {
        intArr[i] = makeElement('int');
    }
    const doubleArr = [];
    for (let i = 0; i < 11; i++) {
        doubleArr[i] = makeElement('double');
    }
    const strArr = [];
    for (let i = 0; i < 11; i++) {
        strArr[i] = makeElement('string');
    }
    const bigintArr = [];
    for (let i = 0; i < 11; i++) {
        bigintArr[i] = makeElement('bigint');
    }

    const targets = [
        1, 1.0, 1.125, '1', 1n
    ];
    const allCases = [
        ['intArr', intArr],
        ['doubleArr', doubleArr],
        ['strArr', strArr],
        ['bigintArr', bigintArr],
    ];
    for (const [name, arr] of allCases) {
        print(`ElementsKind of ${name} = ${ArkTools.getElementsKind(arr)}`);
        for (let i = 0; i < targets.length; i++) {
            const x = targets[i];
            const res1 = arr.indexOf(x);
            const res2 = arr.lastIndexOf(x);
            const res3 = arr.includes(x);
            print(`[${name}] Target = ${x} (${typeof x}):`,
                  `indexOf => ${res1} lastIndexOf => ${res2} includes => ${res3}`);
        }
    }
}

print('======== Regression test ========');
{
    const arr = [0, -0];
    const res1 = arr.indexOf(NaN);
    const res2 = arr.lastIndexOf(NaN);
    const res3 = arr.includes(NaN);
    print(`NaN with [0, -0]: indexOf => ${res1}, lastIndexOf => ${res2}, includes => ${res3}`);
}
{
    const arr = [0, -0, 0n, "NaN", {}];
    const res1 = arr.indexOf(NaN);
    const res2 = arr.lastIndexOf(NaN);
    const res3 = arr.includes(NaN);
    print(`NaN with [0, -0, 0n, "NaN", {}]: indexOf => ${res1}, lastIndexOf => ${res2}, includes => ${res3}`);
}
