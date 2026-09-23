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
//! METHOD loadStringElement
//! HAS LineStringLoadElement
//! HAS LoadSingleCharTableElement
//! HAS CallCommonStub CreateStringBySingleCharCode
//! HAS DeoptIfHClassMismatch
//! HAS_NOT HeapConstant
//! HAS LoadInt32Field
//! HAS I32BitwiseBinary
//! HAS DeoptIfInt32Condition
declare function print(arg: any): string;

function loadStringElement(value, index) {
    return value[index];
}

for (let i = 0; i < 1000; i++) {
    loadStringElement("abc", i % 3);
}

ArkTools.arkSteedCompileSync(loadStringElement);
print(loadStringElement("abc", 1));
print(loadStringElement("A\u4E2DC", 1));
print(loadStringElement("\u0000", 0) === "\u0000");
print(loadStringElement("\u007F", 0).charCodeAt(0) === 0x7F);
print(loadStringElement("\u0080", 0).charCodeAt(0) === 0x80);
print(loadStringElement("abc", 3) === undefined);
print(loadStringElement("abc", -1) === undefined);
print(loadStringElement("abc", "1"));

//! METHOD loadConstantStringElement
//! HAS LoadSingleCharTableElement
//! HAS HeapConstant
//! HAS_NOT LineStringLoadElement
//! HAS_NOT CallCommonStub CreateStringBySingleCharCode
function loadConstantStringElement() {
    let index = 1;
    return "abc"[index];
}

for (let i = 0; i < 1000; i++) {
    loadConstantStringElement();
}
ArkTools.arkSteedCompileSync(loadConstantStringElement);
print(loadConstantStringElement());

//! METHOD loadConstantStringElementByIndex
//! HAS HeapConstant
//! HAS LineStringLoadElement
//! HAS_NOT DeoptIfHClassMismatch
function loadConstantStringElementByIndex(index) {
    return "abc"[index];
}

for (let i = 0; i < 1000; i++) {
    loadConstantStringElementByIndex(i % 3);
}
ArkTools.arkSteedCompileSync(loadConstantStringElementByIndex);
print(loadConstantStringElementByIndex(2));

function loadTreeStringElement(value, index) {
    return value[index];
}

const left = "tree-prefix-";
const right = "string-suffix";
const tree = left + right;
print(ArkTools.isTreeString(tree));
//! METHOD loadTreeStringElement
//! HAS CallCommonStub GetPropertyByValue
for (let i = 0; i < 1000; i++) {
    loadTreeStringElement(tree, i % tree.length);
}

ArkTools.arkSteedCompileSync(loadTreeStringElement);
print(loadTreeStringElement(tree, left.length));
