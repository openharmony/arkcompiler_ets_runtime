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

// === tests/cases/conformance/es6/computedProperties/computedPropertyNamesContextualType6_ES5.ts ===
declare function AssertType(value:any, type:string):void;
interface I<T> {
    [s: string]: T;
}

declare function foo<T>(obj: I<T>): T

foo({
AssertType(foo({    p: "",    0: () => { },    ["hi" + "bye"]: true,    [0 + 1]: 0,    [+"hi"]: [0]}), "union");
AssertType(foo, "<T>(I<T>) => T");
AssertType({    p: "",    0: () => { },    ["hi" + "bye"]: true,    [0 + 1]: 0,    [+"hi"]: [0]}, "{ [string]: union; [number]: union; p: string; 0: () => void; }");

    p: "",
AssertType(p, "string");
AssertType("", "string");

    0: () => { },
AssertType(0, "() => void");
AssertType(() => { }, "() => void");

    ["hi" + "bye"]: true,
AssertType(["hi" + "bye"], "boolean");
AssertType("hi" + "bye", "string");
AssertType("hi", "string");
AssertType("bye", "string");
AssertType(true, "boolean");

    [0 + 1]: 0,
AssertType([0 + 1], "number");
AssertType(0 + 1, "number");
AssertType(0, "int");
AssertType(1, "int");
AssertType(0, "int");

    [+"hi"]: [0]
AssertType([+"hi"], "number[]");
AssertType(+"hi", "number");
AssertType("hi", "string");
AssertType([0], "number[]");
AssertType(0, "int");

});

