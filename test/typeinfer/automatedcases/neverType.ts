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

// === tests/cases/conformance/types/never/neverType.ts ===
declare function AssertType(value:any, type:string):void;
function error(message: string): never {
    throw new Error(message);
AssertType(new Error(message), "Error");
AssertType(Error, "ErrorConstructor");
AssertType(message, "string");
}

function errorVoid(message: string) {
    throw new Error(message);
AssertType(new Error(message), "Error");
AssertType(Error, "ErrorConstructor");
AssertType(message, "string");
}

function fail() {
AssertType(error("Something failed"), "never");
AssertType(error, "(string) => never");
AssertType("Something failed", "string");
    return error("Something failed");
}

function failOrThrow(shouldFail: boolean) {
    if (shouldFail) {
AssertType(shouldFail, "boolean");

AssertType(fail(), "never");
AssertType(fail, "() => never");
        return fail();
    }
    throw new Error();
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");
}

function infiniteLoop1() {
    while (true) {
AssertType(true, "boolean");
    }
}

function infiniteLoop2(): never {
    while (true) {
AssertType(true, "boolean");
    }
}

function move1(direction: "up" | "down") {
    switch (direction) {
AssertType(direction, "union");

        case "up":
AssertType("up", "string");

AssertType(1, "int");
            return 1;

        case "down":
AssertType("down", "string");

AssertType(-1, "int");
AssertType(1, "int");
            return -1; 
    }
AssertType(error("Should never get here"), "never");
AssertType(error, "(string) => never");
AssertType("Should never get here", "string");
    return error("Should never get here");
}

function move2(direction: "up" | "down") {
AssertType(direction === "up" ? 1 :        direction === "down" ? -1 :        error("Should never get here"), "union");
AssertType(direction === "up", "boolean");
AssertType(direction, "union");
AssertType("up", "string");
AssertType(1, "int");
    return direction === "up" ? 1 :

        direction === "down" ? -1 :
AssertType(direction === "down" ? -1 :        error("Should never get here"), "int");
AssertType(direction === "down", "boolean");
AssertType(direction, "string");
AssertType("down", "string");
AssertType(-1, "int");
AssertType(1, "int");

        error("Should never get here");
AssertType(error("Should never get here"), "never");
AssertType(error, "(string) => never");
AssertType("Should never get here", "string");
}

function check<T>(x: T | undefined) {
AssertType(x || error("Undefined value"), "NonNullable<T>");
AssertType(x, "union");
AssertType(error("Undefined value"), "never");
AssertType(error, "(string) => never");
AssertType("Undefined value", "string");
    return x || error("Undefined value");
}

class C {
    void1() {
        throw new Error();
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");
    }
    void2() {
        while (true) {
AssertType(true, "boolean");
}
    }
    never1(): never {
        throw new Error();
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");
    }
    never2(): never {
        while (true) {
AssertType(true, "boolean");
}
    }
}

function f1(x: string | number) {
    if (typeof x === "boolean") {
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("boolean", "string");

        x;  // never
AssertType(x, "never");
    }
}

function f2(x: string | number) {
    while (true) {
AssertType(true, "boolean");

        if (typeof x === "boolean") {
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("boolean", "string");

AssertType(x, "never");
            return x;  // never
        }
    }
}

function test(cb: () => string) {
    let s = cb();
AssertType(s, "string");
AssertType(cb(), "string");
AssertType(cb, "() => string");

AssertType(s, "string");
    return s;
}

let errorCallback = () => error("Error callback");
AssertType(errorCallback, "() => never");
AssertType(() => error("Error callback"), "() => never");
AssertType(error("Error callback"), "never");
AssertType(error, "(string) => never");
AssertType("Error callback", "string");

test(() => "hello");
AssertType(test(() => "hello"), "string");
AssertType(test, "(() => string) => string");
AssertType(() => "hello", "() => string");
AssertType("hello", "string");

test(() => fail());
AssertType(test(() => fail()), "string");
AssertType(test, "(() => string) => string");
AssertType(() => fail(), "() => never");
AssertType(fail(), "never");
AssertType(fail, "() => never");

test(() => { throw new Error(); })
AssertType(test(() => { throw new Error(); }), "string");
AssertType(test, "(() => string) => string");
AssertType(() => { throw new Error(); }, "() => never");
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");

test(errorCallback);
AssertType(test(errorCallback), "string");
AssertType(test, "(() => string) => string");
AssertType(errorCallback, "() => never");


