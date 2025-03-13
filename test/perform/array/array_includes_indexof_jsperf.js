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

/**
 * Based on the following test cases in JSPerf:
 *   - https://gitee.com/dov1s/arkjs-perf-test/blob/builtins_test1110/js-perf-test/Array/includes.js
 *   - https://gitee.com/dov1s/arkjs-perf-test/blob/builtins_test1110/js-perf-test/Array/index-of.js
 *   - https://gitee.com/dov1s/arkjs-perf-test/blob/builtins_test1110/js-perf-test/Array/last-index-of.js
 */

const REP_COUNT = 1_000_000;

const result = {}

function addToResult(fnName, subCaseName, timeInMs) {
    print(`${fnName}.${subCaseName}: ${timeInMs.toFixed(3)} ms.`);
    result[`${fnName}.${subCaseName}`] = timeInMs;
}

function noOpFn(_) { /* no-op */ }

const fnNames = ['includes', 'indexOf', 'lastIndexOf'];
for (let i = 0; i < fnNames.length; i++) {
    print(`==== Array.prototype.${fnNames[i]} ====`);
    const fnName = fnNames[i];

    let array1 = new Array();
    let array2 = new Array();
    let array3 = new Array();
    let array4 = new Array();
    let array_size = 1000;

    for (let i = 0; i < array_size; ++i) array1[i] = i;
    for (let i = 0; i < array_size; ++i) array2[i] = i * 0.9;
    for (let i = 0; i < array_size; i++) array3[i] = `string.${i}`;
    for (let i = 0; i < array_size; i++) array4[i] = { ["obj" + i]: i };

    const arrays = [array1, array2, array3, array4];
    for (let i = 0; i < arrays.length; i++) {
        print(`ElementsKind of array${i + 1} = ${ArkTools.getElementsKind(arrays[i])}`);
    }

    addToResult(fnNames[i], 'int', (() => {
        const s = ArkTools.timeInUs();
        for (let i = 0; i < REP_COUNT; i++) {
            array1[fnName](i);
        }
        const t = ArkTools.timeInUs();
        for (let i = 0; i < REP_COUNT; i++) {
            array1[fnName];
            i;
        }
        const u = ArkTools.timeInUs();
        return (2 * t - s - u) / 1000.0;
    })());
    addToResult(fnNames[i], 'double', (() => {
        const s = ArkTools.timeInUs();
        for (let i = 0; i < REP_COUNT; i++) {
            array2[fnName](i);
        }
        const t = ArkTools.timeInUs();
        for (let i = 0; i < REP_COUNT; i++) {
            array2[fnName];
            i;
        }
        const u = ArkTools.timeInUs();
        return (2 * t - s - u) / 1000.0;
    })());
    addToResult(fnNames[i], 'string', (() => {
        const s = ArkTools.timeInUs();
        for (let i = 0; i < REP_COUNT; i++) {
            array3[fnName](`string.${i}`);
        }
        const t = ArkTools.timeInUs();
        for (let i = 0; i < REP_COUNT; i++) {
            array3[fnName];
            `string.${i}`;
        }
        const u = ArkTools.timeInUs();
        return (2 * t - s - u) / 1000.0;
    })());
    addToResult(fnNames[i], 'object', (() => {
        const s = ArkTools.timeInUs();
        for (let i = 0; i < REP_COUNT; i++) {
            array4[fnName]({ ['obj' + i]: i });
        }
        const t = ArkTools.timeInUs();
        for (let i = 0; i < REP_COUNT; i++) {
            array4[fnName];
            ({ ['obj' + i]: i });
        }
        const u = ArkTools.timeInUs();
        return (2 * t - s - u) / 1000.0;
    })());
}

print(JSON.stringify(result, null, 4));
