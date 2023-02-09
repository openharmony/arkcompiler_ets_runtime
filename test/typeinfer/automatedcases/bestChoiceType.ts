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

// === tests/cases/compiler/bestChoiceType.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #10041

(''.match(/ /) || []).map(s => s.toLowerCase());
AssertType((''.match(/ /) || []).map(s => s.toLowerCase()), "any[]");
AssertType((''.match(/ /) || []).map, "union");
AssertType(s => s.toLowerCase(), "(any) => any");
AssertType(s, "any");
AssertType(s.toLowerCase(), "any");
AssertType(s.toLowerCase, "any");

// Similar cases

function f1() {
    let x = ''.match(/ /);
AssertType(x, "union");
AssertType(''.match(/ /), "union");
AssertType(''.match, "(union) => union");
AssertType('', "string");
AssertType(/ /, "RegExp");

    let y = x || [];
AssertType(y, "union");
AssertType(x || [], "union");
AssertType(x, "union");
AssertType([], "[]");

    let z = y.map(s => s.toLowerCase());
AssertType(z, "any[]");
AssertType(y.map(s => s.toLowerCase()), "any[]");
AssertType(y.map, "union");
AssertType(s => s.toLowerCase(), "(any) => any");
AssertType(s, "any");
AssertType(s.toLowerCase(), "any");
AssertType(s.toLowerCase, "any");
}

function f2() {
    let x = ''.match(/ /);
AssertType(x, "union");
AssertType(''.match(/ /), "union");
AssertType(''.match, "(union) => union");
AssertType('', "string");
AssertType(/ /, "RegExp");

    let y = x ? x : [];
AssertType(y, "union");
AssertType(x ? x : [], "union");
AssertType(x, "union");
AssertType(x, "RegExpMatchArray");
AssertType([], "never[]");

    let z = y.map(s => s.toLowerCase());
AssertType(z, "any[]");
AssertType(y.map(s => s.toLowerCase()), "any[]");
AssertType(y.map, "union");
AssertType(s => s.toLowerCase(), "(any) => any");
AssertType(s, "any");
AssertType(s.toLowerCase(), "any");
AssertType(s.toLowerCase, "any");
}


