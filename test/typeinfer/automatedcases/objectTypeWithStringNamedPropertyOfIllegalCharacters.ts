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

// === tests/cases/conformance/types/members/objectTypeWithStringNamedPropertyOfIllegalCharacters.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    "   ": number;
    "a   b": string;
    "~!@#$%^&*()_+{}|:'<>?\/.,`": number;
    "a\a": number;
    static "a ": number
}

let c: C;
AssertType(c, "C");

let r = c["   "];
AssertType(r, "number");
AssertType(c["   "], "number");
AssertType(c, "C");
AssertType("   ", "string");

let r2 = c["    "];
AssertType(r2, "error");
AssertType(c["    "], "error");
AssertType(c, "C");
AssertType("    ", "string");

let r3 = c["a   b"];
AssertType(r3, "string");
AssertType(c["a   b"], "string");
AssertType(c, "C");
AssertType("a   b", "string");

// BUG 817263
let r4 = c["~!@#$%^&*()_+{}|:'<>?\/.,`"];
AssertType(r4, "number");
AssertType(c["~!@#$%^&*()_+{}|:'<>?\/.,`"], "number");
AssertType(c, "C");
AssertType("~!@#$%^&*()_+{}|:'<>?\/.,`", "string");

interface I {
    "   ": number;
    "a   b": string;
    "~!@#$%^&*()_+{}|:'<>?\/.,`": number;
}

let i: I;
AssertType(i, "I");

let r = i["   "];
AssertType(r, "number");
AssertType(i["   "], "number");
AssertType(i, "I");
AssertType("   ", "string");

let r2 = i["    "];
AssertType(r2, "error");
AssertType(i["    "], "error");
AssertType(i, "I");
AssertType("    ", "string");

let r3 = i["a   b"];
AssertType(r3, "string");
AssertType(i["a   b"], "string");
AssertType(i, "I");
AssertType("a   b", "string");

// BUG 817263
let r4 = i["~!@#$%^&*()_+{}|:'<>?\/.,`"];
AssertType(r4, "number");
AssertType(i["~!@#$%^&*()_+{}|:'<>?\/.,`"], "number");
AssertType(i, "I");
AssertType("~!@#$%^&*()_+{}|:'<>?\/.,`", "string");


let a: {
AssertType(a, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");

    "   ": number;
AssertType("   ", "number");

    "a   b": string;
AssertType("a   b", "string");

    "~!@#$%^&*()_+{}|:'<>?\/.,`": number;
AssertType("~!@#$%^&*()_+{}|:'<>?\/.,`", "number");
}

let r = a["   "];
AssertType(r, "number");
AssertType(a["   "], "number");
AssertType(a, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");
AssertType("   ", "string");

let r2 = a["    "];
AssertType(r2, "error");
AssertType(a["    "], "error");
AssertType(a, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");
AssertType("    ", "string");

let r3 = a["a   b"];
AssertType(r3, "string");
AssertType(a["a   b"], "string");
AssertType(a, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");
AssertType("a   b", "string");

// BUG 817263
let r4 = a["~!@#$%^&*()_+{}|:'<>?\/.,`"];
AssertType(r4, "number");
AssertType(a["~!@#$%^&*()_+{}|:'<>?\/.,`"], "number");
AssertType(a, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");
AssertType("~!@#$%^&*()_+{}|:'<>?\/.,`", "string");

let b = {
AssertType(b, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");
AssertType({    "   ": 1,    "a   b": "",    "~!@#$%^&*()_+{}|:'<>?\/.,`": 1,}, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");

    "   ": 1,
AssertType("   ", "number");
AssertType(1, "int");

    "a   b": "",
AssertType("a   b", "string");
AssertType("", "string");

    "~!@#$%^&*()_+{}|:'<>?\/.,`": 1,
AssertType("~!@#$%^&*()_+{}|:'<>?\/.,`", "number");
AssertType(1, "int");
}

let r = b["   "];
AssertType(r, "number");
AssertType(b["   "], "number");
AssertType(b, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");
AssertType("   ", "string");

let r2 = b["    "];
AssertType(r2, "error");
AssertType(b["    "], "error");
AssertType(b, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");
AssertType("    ", "string");

let r3 = b["a   b"];
AssertType(r3, "string");
AssertType(b["a   b"], "string");
AssertType(b, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");
AssertType("a   b", "string");

// BUG 817263
let r4 = b["~!@#$%^&*()_+{}|:'<>?\/.,`"];
AssertType(r4, "number");
AssertType(b["~!@#$%^&*()_+{}|:'<>?\/.,`"], "number");
AssertType(b, "{ "   ": number; "a   b": string; "~!@#$%^&*()_+{}|:'<>?/.,`": number; }");
AssertType("~!@#$%^&*()_+{}|:'<>?\/.,`", "string");


