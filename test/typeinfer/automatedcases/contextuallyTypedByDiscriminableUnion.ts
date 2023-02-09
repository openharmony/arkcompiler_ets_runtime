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

// === tests/cases/compiler/contextuallyTypedByDiscriminableUnion.ts ===
declare function AssertType(value:any, type:string):void;
type ADT = {
    kind: "a",
    method(x: string): number;
} | {
    kind: "b",
    method(x: number): string;
};


function invoke(item: ADT) {
    if (item.kind === "a") {
AssertType(item.kind === "a", "boolean");
AssertType(item.kind, "union");
AssertType("a", "string");

        item.method("");
AssertType(item.method(""), "number");
AssertType(item.method, "(string) => number");
AssertType("", "string");
    }
    else {
        item.method(42);
AssertType(item.method(42), "string");
AssertType(item.method, "(number) => string");
AssertType(42, "int");
    }
}

invoke({
AssertType(invoke, "(ADT) => void");
AssertType({    kind: "a",    method(a) {        return +a;    }}, "{ kind: "a"; method(string): number; }");
AssertType(invoke({    kind: "a",    method(a) {        return +a;    }}), "void");

    kind: "a",
AssertType(kind, "string");
AssertType("a", "string");

    method(a) {
AssertType(method, "(string) => number");
AssertType(a, "string");

AssertType(+a, "number");
AssertType(a, "string");
        return +a;
    }
});


