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

// === tests/cases/conformance/jsdoc/declarations/index.js ===
declare function AssertType(value:any, type:string):void;
/** @enum {string} */
export const Target = {
AssertType(Target, "{ START: string; MIDDLE: string; END: string; OK_I_GUESS: number; }");
AssertType({    START: "start",    MIDDLE: "middle",    END: "end",    /** @type {number} */    OK_I_GUESS: 2}, "{ START: string; MIDDLE: string; END: string; OK_I_GUESS: number; }");

    START: "start",
AssertType(START, "string");
AssertType("start", "string");

    MIDDLE: "middle",
AssertType(MIDDLE, "string");
AssertType("middle", "string");

    END: "end",
AssertType(END, "string");
AssertType("end", "string");

    /** @type {number} */
    OK_I_GUESS: 2
AssertType(OK_I_GUESS, "number");
AssertType(2, "int");
}
/** @enum number */
export const Second = {
AssertType(Second, "{ OK: number; FINE: number; }");
AssertType({    OK: 1,    /** @type {number} */    FINE: 2,}, "{ OK: number; FINE: number; }");

    OK: 1,
AssertType(OK, "number");
AssertType(1, "int");

    /** @type {number} */
    FINE: 2,
AssertType(FINE, "number");
AssertType(2, "int");
}
/** @enum {function(number): number} */
export const Fs = {
AssertType(Fs, "{ ADD1: (any) => any; ID: (any) => any; SUB1: (any) => number; }");
AssertType({    ADD1: n => n + 1,    ID: n => n,    SUB1: n => n - 1}, "{ ADD1: (any) => any; ID: (any) => any; SUB1: (any) => number; }");

    ADD1: n => n + 1,
AssertType(ADD1, "(any) => any");
AssertType(n => n + 1, "(any) => any");
AssertType(n, "any");
AssertType(n + 1, "any");
AssertType(n, "any");
AssertType(1, "int");

    ID: n => n,
AssertType(ID, "(any) => any");
AssertType(n => n, "(any) => any");
AssertType(n, "any");
AssertType(n, "any");

    SUB1: n => n - 1
AssertType(SUB1, "(any) => number");
AssertType(n => n - 1, "(any) => number");
AssertType(n, "any");
AssertType(n - 1, "number");
AssertType(n, "any");
AssertType(1, "int");
}

/**
 * @param {Target} t
 * @param {Second} s
 * @param {Fs} f
 */
export function consume(t,s,f) {
    /** @type {string} */
    let str = t
AssertType(str, "string");
AssertType(t, "string");

    /** @type {number} */
    let num = s
AssertType(num, "number");
AssertType(s, "number");

    /** @type {(n: number) => number} */
    let fun = f
AssertType(fun, "(number) => number");
AssertType(f, "Fs");

    /** @type {Target} */
    let v = Target.START
AssertType(v, "string");
AssertType(Target.START, "string");

    v = 'something else' // allowed, like Typescript's classic enums and unlike its string enums
AssertType(v = 'something else', "string");
AssertType(v, "string");
AssertType('something else', "string");
}
/** @param {string} s */
export function ff(s) {
    // element access with arbitrary string is an error only with noImplicitAny
    if (!Target[s]) {
AssertType(!Target[s], "boolean");
AssertType(Target[s], "error");
AssertType(Target, "{ START: string; MIDDLE: string; END: string; OK_I_GUESS: number; }");
AssertType(s, "string");

AssertType(null, "null");
        return null
    }
    else {
AssertType(Target[s], "error");
AssertType(Target, "{ START: string; MIDDLE: string; END: string; OK_I_GUESS: number; }");
AssertType(s, "string");
        return Target[s]
    }
}


