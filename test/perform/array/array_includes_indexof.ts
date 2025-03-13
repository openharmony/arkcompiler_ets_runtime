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

declare function print(arg:any): string;

declare interface ArkTools {
    timeInUs(arg: any): number;
    isStableJsArray(arr: any[]): boolean;
    getElementsKind(arr: any[]): number;
}

const REP_COUNT = 100_000;

function initializeDenseArray(target: any, len: number, a: number, b: number, intOnly = true) {
    for (let i = 0; i < len; i++) {
        target[i] = (i * a + b) % len;
        if (!intOnly && i % 2 == 0) {
            target[i] += i / len; // int : double = 50% : 50%
        }
    }
}

function initializeSparseArray(target: any, len: number, nNonHole: number, a: number, b: number, intOnly = true) {
    let last = 0;
    const getNextElement = () => {
        const res = (last * a + b) % len;
        last = res;
        return res;
    }
    for (let i = 0; i < nNonHole; i++) {
        const index = getNextElement();
        let value = getNextElement();
        if (!intOnly && i % 2 == 0) {
            value += i / len; // int : double = 50% : 50%
        }
        target[index] = value;
    }
}

const result = {};

function testArrayFunc(fnName: string, caseName: string, arr: number[]) {
    print(`-------- Starts new test of Array.prototype.${fnName}()  --------`)
    print(`Is arr stable array? ${ArkTools.isStableJsArray(arr)}`);
    print(`arr.length = ${arr.length}. ElementsKind of arr: ${ArkTools.getElementsKind(arr)}`);
    const someObj = { x: 1, y: 2, valueOf() { return 3; } };

    let start: number;
    let end: number;
    let endExtra: number;

    start = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        arr[fnName](i);
    }
    end = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        arr[fnName];
        i;
    }
    endExtra = ArkTools.timeInUs();
    const durationI = result[`${fnName}.${caseName}.intIndex`] = (2 * end - start - endExtra) / 1000;
    print(`\tsearchElement is integer: ${durationI.toFixed(3)} ms.`);

    start = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        arr[fnName](1.1 * i); // Expects early exit when target is not within int32 (90% chance)
    }
    end = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        arr[fnName];
        1.1 * i;
    }
    endExtra = ArkTools.timeInUs();
    const durationF = result[`${fnName}.${caseName}.doubleIndex`] = (2 * end - start - endExtra) / 1000;
    print(`\tsearchElement is double: ${durationF.toFixed(3)} ms.`);

    start = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        arr[fnName]("1"); // Expects early exit
    }
    end = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        arr[fnName];
        "1";
    }
    endExtra = ArkTools.timeInUs();
    const durationS = result[`${fnName}.${caseName}.stringIndex`] = (2 * end - start - endExtra) / 1000;
    print(`\tsearchElement is string: ${durationS.toFixed(3)} ms.`);

    start = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        arr[fnName](someObj); // Expects early exit
    }
    end = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        arr[fnName];
        someObj;
    }
    endExtra = ArkTools.timeInUs();
    const durationO = result[`${fnName}.${caseName}.objectIndex`] = (2 * end - start - endExtra) / 1000;
    print(`\tsearchElement is object: ${durationO.toFixed(3)} ms.`);
}

for (const fnName of ['includes', 'indexOf', 'lastIndexOf']) {
    {
        const empty = [];
        testArrayFunc(fnName, 'empty', empty);
    }
    {
        const [ARR_LENGTH, N_NOT_HOLE_ELEMENTS, GEN_A, GEN_B] = [1499, 100, 233, 1];
        const arr = new Array<number>(ARR_LENGTH);
        initializeSparseArray(arr, ARR_LENGTH, N_NOT_HOLE_ELEMENTS, GEN_A, GEN_B);
        testArrayFunc(fnName, `sparse.len${ARR_LENGTH}.generic`, arr); // ElementKind = 31 (GENERIC)
    }
    {
        const [ARR_LENGTH, GEN_A, GEN_B] = [10007, 233, 1];
        const arr = new Array<number>(ARR_LENGTH);
        initializeDenseArray(arr, ARR_LENGTH, GEN_A, GEN_B);
        testArrayFunc(fnName, `dense.len${ARR_LENGTH}.generic`, arr); // ElementKind = 31 (GENERIC)
    }
    for (const intOnly of [true, false]) {
        {
            const [ARR_LENGTH, GEN_A, GEN_B] = [17, 5, 1];
            const arr: number[] = [];
            initializeDenseArray(arr, ARR_LENGTH, GEN_A, GEN_B, intOnly);
            testArrayFunc(fnName, `dense.len${ARR_LENGTH}.${intOnly ? 'int' : 'number'}`, arr);
        }
        {
            const [ARR_LENGTH, N_NOT_HOLE_ELEMENTS, GEN_A, GEN_B] = [1499, 100, 233, 1];
            const arr: number[] = [];
            initializeSparseArray(arr, ARR_LENGTH, N_NOT_HOLE_ELEMENTS, GEN_A, GEN_B, intOnly);
            testArrayFunc(fnName, `sparse.len${ARR_LENGTH}.hole${intOnly ? 'Int' : 'Number'}`, arr);
        }
        {
            const [ARR_LENGTH, GEN_A, GEN_B] = [10007, 233, 1];
            const arr: number[] = [];
            initializeDenseArray(arr, ARR_LENGTH, GEN_A, GEN_B, intOnly);
            testArrayFunc(fnName, `dense.len${ARR_LENGTH}.${intOnly ? 'int' : 'number'}`, arr);
        }
    }
}

print(JSON.stringify(result, null, 4));
