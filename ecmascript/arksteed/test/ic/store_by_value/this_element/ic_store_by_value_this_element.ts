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

//! METHOD storeThisElement
//! HAS StoreTaggedElement
//! HAS_NOT CallCommonStub SetPropertyByValue
function storeThisElement(index, value) {
    this[index] = value;
    return this[index];
}

let array = [1, 2, 3];
for (let i = 0; i < 20; i++) {
    storeThisElement.call(array, i % array.length, i);
}
ArkTools.arkSteedCompileSync(storeThisElement);
print(storeThisElement.call(array, 1, 41));

//! METHOD storeThisInt32Element
//! HAS StoreIntTypedArrayElement
//! HAS_NOT CallCommonStub SetPropertyByValue
function storeThisInt32Element(index, value) {
    this[index] = value;
    return this[index];
}

let int32Array = new Int32Array(4);
for (let i = 0; i < 20; i++) {
    storeThisInt32Element.call(int32Array, i % int32Array.length, i);
}
ArkTools.arkSteedCompileSync(storeThisInt32Element);
print(storeThisInt32Element.call(int32Array, 2, -77));
