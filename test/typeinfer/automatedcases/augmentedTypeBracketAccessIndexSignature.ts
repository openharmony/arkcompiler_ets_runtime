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

// === tests/cases/conformance/types/members/augmentedTypeBracketAccessIndexSignature.ts ===
declare function AssertType(value:any, type:string):void;
interface Foo { a }
interface Bar { b }

interface Object {
    [n: number]: Foo;
}

interface Function {
    [n: number]: Bar;
}

let a = {}[0]; // Should be Foo
AssertType(a, "error");
AssertType({}[0], "error");
AssertType({}, "{}");
AssertType(0, "int");

let b = (() => { })[0]; // Should be Bar
AssertType(b, "error");
AssertType((() => { })[0], "error");
AssertType((() => { }), "() => void");
AssertType(() => { }, "() => void");
AssertType(0, "int");


