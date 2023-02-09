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

// === tests/cases/compiler/nonNullableTypes1.ts ===
declare function AssertType(value:any, type:string):void;
function f1<T>(x: T) {
    let y = x || "hello";  // NonNullable<T> | string
AssertType(y, "union");
AssertType(x || "hello", "union");
AssertType(x, "T");
AssertType("hello", "string");
}

function error(): never {
    throw new Error();
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");
}

function f2<T>(x: T) {  // NonNullable<T>
AssertType(x || error(), "NonNullable<T>");
AssertType(x, "T");
AssertType(error(), "never");
AssertType(error, "() => never");
    return x || error();
}

function f3(x: unknown) {
    let y = x!;  // {
AssertType(y, "{}");

AssertType(x!, "{}");

AssertType(x, "unknown");
}
}

function f4<T extends { x: string } | undefined>(obj: T) {
    if (obj?.x === "hello") {
AssertType(obj?.x === "hello", "boolean");
AssertType(obj?.x, "union");
AssertType("hello", "string");

        obj;  // NonNullable<T>
AssertType(obj, "NonNullable<T>");
    }
    if (obj?.x) {
AssertType(obj?.x, "union");

        obj;  // NonNullable<T>
AssertType(obj, "NonNullable<T>");
    }
    if (typeof obj?.x === "string") {
AssertType(typeof obj?.x === "string", "boolean");
AssertType(typeof obj?.x, "union");
AssertType(obj?.x, "union");
AssertType("string", "string");

        obj;  // NonNullable<T>
AssertType(obj, "NonNullable<T>");
    }
}

class A {
    x = "hello";
    foo() {
        let zz = this?.x;  // string
AssertType(zz, "string");
AssertType(this?.x, "string");
AssertType(this, "this");
    }
}


