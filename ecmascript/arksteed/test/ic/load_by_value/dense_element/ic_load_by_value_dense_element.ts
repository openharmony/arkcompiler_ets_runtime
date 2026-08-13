/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// @ts-nocheck
//! PARAMS --compiler-jit-hotness-threshold=10000
//! METHOD loadDense
//! HAS LoadTaggedElement
//! HAS LoadInt32Field
//! HAS DeoptIfInt32Condition
//! HAS DeoptIfTaggedCondition
//! HAS_NOT CallCommonStub GetPropertyByValue
declare function print(arg: any): string;

function loadDense(array, index) {
    return array[index];
}

const values = [10, 20, 30, 40];
for (let i = 0; i < 1000; i++) {
    loadDense(values, i & 3);
}

ArkTools.arkSteedCompileSync(loadDense);
print(loadDense(values, 2));
print(loadDense(values, 99) === undefined);

function loadNegative(array, index) {
    return array[index];
}

for (let i = 0; i < 1000; i++) {
    loadNegative(values, i & 3);
}
ArkTools.arkSteedCompileSync(loadNegative);
print(loadNegative(values, -1) === undefined);

function loadHole(array, index) {
    return array[index];
}

const valuesWithHole = [11, , 33];
Array.prototype[1] = 77;
for (let i = 0; i < 1000; i++) {
    loadHole(valuesWithHole, 0);
}
ArkTools.arkSteedCompileSync(loadHole);
print(loadHole(valuesWithHole, 1));
delete Array.prototype[1];

function loadStringIndex(array, index) {
    return array[index];
}

for (let i = 0; i < 1000; i++) {
    loadStringIndex(values, i & 3);
}
ArkTools.arkSteedCompileSync(loadStringIndex);
print(loadStringIndex(values, "2"));

function loadChangedReceiver(object, index) {
    return object[index];
}

for (let i = 0; i < 1000; i++) {
    loadChangedReceiver(values, i & 3);
}
ArkTools.arkSteedCompileSync(loadChangedReceiver);
print(loadChangedReceiver({0: 55}, 0));

function loadDenseObject(object, index) {
    return object[index];
}

const denseObject = {0: "zero", 1: "one"};
for (let i = 0; i < 1000; i++) {
    loadDenseObject(denseObject, i & 1);
}
ArkTools.arkSteedCompileSync(loadDenseObject);
print(loadDenseObject(denseObject, 1));
