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

// === tests/cases/compiler/inferStringLiteralUnionForBindingElement.ts ===
declare function AssertType(value:any, type:string):void;
declare function func<T extends string>(arg: { keys: T[] }): { readonly keys: T[]; readonly firstKey: T; };

function func1() {
    const { firstKey } = func({keys: ["aa", "bb"]})
AssertType(firstKey, "union");
AssertType(func({keys: ["aa", "bb"]}), "{ readonly keys: (union)[]; readonly firstKey: union; }");
AssertType(func, "<T extends string>({ keys: T[]; }) => { readonly keys: T[]; readonly firstKey: T; }");
AssertType({keys: ["aa", "bb"]}, "{ keys: (union)[]; }");
AssertType(keys, "(union)[]");
AssertType(["aa", "bb"], "(union)[]");
AssertType("aa", "string");
AssertType("bb", "string");

    const a: "aa" | "bb" = firstKey;
AssertType(a, "union");
AssertType(firstKey, "union");

    const { keys } = func({keys: ["aa", "bb"]})
AssertType(keys, "(union)[]");
AssertType(func({keys: ["aa", "bb"]}), "{ readonly keys: (union)[]; readonly firstKey: union; }");
AssertType(func, "<T extends string>({ keys: T[]; }) => { readonly keys: T[]; readonly firstKey: T; }");
AssertType({keys: ["aa", "bb"]}, "{ keys: (union)[]; }");
AssertType(keys, "(union)[]");
AssertType(["aa", "bb"], "(union)[]");
AssertType("aa", "string");
AssertType("bb", "string");

    const b: ("aa" | "bb")[] = keys;
AssertType(b, "(union)[]");
AssertType(keys, "(union)[]");
}
  
function func2() {
    const { keys, firstKey } = func({keys: ["aa", "bb"]})
AssertType(keys, "(union)[]");
AssertType(firstKey, "union");
AssertType(func({keys: ["aa", "bb"]}), "{ readonly keys: (union)[]; readonly firstKey: union; }");
AssertType(func, "<T extends string>({ keys: T[]; }) => { readonly keys: T[]; readonly firstKey: T; }");
AssertType({keys: ["aa", "bb"]}, "{ keys: (union)[]; }");
AssertType(keys, "(union)[]");
AssertType(["aa", "bb"], "(union)[]");
AssertType("aa", "string");
AssertType("bb", "string");

    const a: "aa" | "bb" = firstKey;
AssertType(a, "union");
AssertType(firstKey, "union");

    const b: ("aa" | "bb")[] = keys;
AssertType(b, "(union)[]");
AssertType(keys, "(union)[]");
}

function func3() {
    const x = func({keys: ["aa", "bb"]})
AssertType(x, "{ readonly keys: (union)[]; readonly firstKey: union; }");
AssertType(func({keys: ["aa", "bb"]}), "{ readonly keys: (union)[]; readonly firstKey: union; }");
AssertType(func, "<T extends string>({ keys: T[]; }) => { readonly keys: T[]; readonly firstKey: T; }");
AssertType({keys: ["aa", "bb"]}, "{ keys: (union)[]; }");
AssertType(keys, "(union)[]");
AssertType(["aa", "bb"], "(union)[]");
AssertType("aa", "string");
AssertType("bb", "string");

    const a: "aa" | "bb" = x.firstKey;
AssertType(a, "union");
AssertType(x.firstKey, "union");

    const b: ("aa" | "bb")[] = x.keys;
AssertType(b, "(union)[]");
AssertType(x.keys, "(union)[]");
}


