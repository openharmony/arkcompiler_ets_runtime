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
//! HAS DeoptIfHClassNotIn
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

function makeLargeDenseArray() {
    return [
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
        52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1, 0,-1,-1,
        -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
        15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
        -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
        41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
    ];
}

function loadAcrossArrayLiteralTier(array, index) {
    return array[index];
}

const interpretedLargeArray = makeLargeDenseArray();
for (let i = 0; i < 1000; i++) {
    loadAcrossArrayLiteralTier(interpretedLargeArray, 65 + (i & 15));
}
ArkTools.arkSteedCompileSync(loadAcrossArrayLiteralTier);
ArkTools.arkSteedCompileSync(makeLargeDenseArray);
const compiledLargeArray = makeLargeDenseArray();
print(loadAcrossArrayLiteralTier(compiledLargeArray, 65));
