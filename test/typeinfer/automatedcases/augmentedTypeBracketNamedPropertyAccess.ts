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

// === tests/cases/compiler/augmentedTypeBracketNamedPropertyAccess.ts ===
declare function AssertType(value:any, type:string):void;
interface Object {
    data: number;
}
interface Function {
    functionData: string;
}
let o = {};
AssertType(o, "{}");
AssertType({}, "{}");

let f = function () { };
AssertType(f, "() => void");
AssertType(function () { }, "() => void");

let r1 = o['data']; // Should be number
AssertType(r1, "number");
AssertType(o['data'], "number");
AssertType(o, "{}");
AssertType('data', "string");

let r2 = o['functionData']; // Should be any (no property found)
AssertType(r2, "error");
AssertType(o['functionData'], "error");
AssertType(o, "{}");
AssertType('functionData', "string");

let r3 = f['functionData']; // Should be string
AssertType(r3, "string");
AssertType(f['functionData'], "string");
AssertType(f, "() => void");
AssertType('functionData', "string");

let r4 = f['data']; // Should be number
AssertType(r4, "number");
AssertType(f['data'], "number");
AssertType(f, "() => void");
AssertType('data', "string");


