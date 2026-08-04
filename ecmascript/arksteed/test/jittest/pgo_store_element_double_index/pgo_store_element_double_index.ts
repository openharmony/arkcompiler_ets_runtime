/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// @ts-nocheck
declare function print(value: any): void;

function makeDoubleIndex(value) {
    return value - 0.25;
}

function storeWithNumberIndex(array, index, value) {
    array[index] = value;
    return array[index];
}

function storeWithNaNIndex(array, index, value) {
    array[index] = value;
    return array[index];
}

function storeWithInfinityIndex(array, index, value) {
    array[index] = value;
    return array[index];
}

let array = [0, 1, 2];
for (let i = 0; i < 20; i++) {
    storeWithNumberIndex(array, 1, i);
    storeWithNaNIndex(array, 1, i);
    storeWithInfinityIndex(array, 1, i);
}
ArkTools.arkSteedCompileSync(storeWithNumberIndex);
ArkTools.arkSteedCompileSync(storeWithNaNIndex);
ArkTools.arkSteedCompileSync(storeWithInfinityIndex);

let exactDouble = makeDoubleIndex(1.25);
print(storeWithNumberIndex(array, exactDouble, 61));
let fractionalDouble = makeDoubleIndex(1.5);
print(storeWithNumberIndex(array, fractionalDouble, 62));
print(storeWithNaNIndex(array, Number.NaN, 63));
print(storeWithInfinityIndex(array, Infinity, 64));
print(array[1]);
