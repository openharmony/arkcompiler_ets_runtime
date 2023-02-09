/*
* Copyright (c) Microsoft Corporation. All rights reserved.
* Copyright (c) 2023 Huawei Device Co., Ltd.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/conformance/controlFlow/controlFlowForInStatement2.ts ===
declare function AssertType(value:any, type:string):void;
const keywordA = 'a';
AssertType(keywordA, "string");
AssertType('a', "string");

const keywordB = 'b';
AssertType(keywordB, "string");
AssertType('b', "string");

type A = { [keywordA]: number };
type B = { [keywordB]: string };

declare const c: A | B;
AssertType(c, "union");

if ('a' in c) {
    c; // narrowed to `A`
AssertType(c, "A");
}

if (keywordA in c) {
    c; // also narrowed to `A`
AssertType(c, "A");
}

let stringB: string = 'b';
AssertType(stringB, "string");
AssertType('b', "string");

if ((stringB as 'b') in c) {
    c; // narrowed to `B`
AssertType(c, "B");
}

if ((stringB as ('a' | 'b')) in c) {
    c; // not narrowed
AssertType(c, "union");
}

