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

// === tests/cases/conformance/types/typeRelationships/bestCommonType/functionWithMultipleReturnStatements2.ts ===
declare function AssertType(value:any, type:string):void;
// return type of a function with multiple returns is the BCT of each return statement
// no errors expected here

function f1() {
    if (true) {
AssertType(true, "boolean");

AssertType(1, "int");
        return 1;

    } else {
AssertType(null, "null");
        return null;
    }
}

function f2() {
    if (true) {
AssertType(true, "boolean");

AssertType(1, "int");
        return 1;

    } else if (false) {
AssertType(false, "boolean");

AssertType(null, "null");
        return null;

    } else {
AssertType(2, "int");
        return 2;
    }
}

function f4() {
    try {
AssertType(1, "int");
        return 1;
    }
    catch (e) {
AssertType(e, "any");

AssertType(undefined, "undefined");
        return undefined;
    }
    finally {
AssertType(1, "int");
        return 1;
    }
}

function f5() {
AssertType(1, "int");
    return 1;

AssertType(new Object(), "Object");
AssertType(Object, "ObjectConstructor");
    return new Object();
}

function f6<T>(x: T) {
    if (true) {
AssertType(true, "boolean");

AssertType(x, "T");
        return x;

    } else {
AssertType(null, "null");
        return null;
    }
}

let a: { x: number; y?: number };
AssertType(a, "{ x: number; y?: number; }");
AssertType(x, "number");
AssertType(y, "number");

let b: { x: number; z?: number };
AssertType(b, "{ x: number; z?: number; }");
AssertType(x, "number");
AssertType(z, "number");

// returns typeof a
function f9() {
    if (true) {
AssertType(true, "boolean");

AssertType(a, "{ x: number; y?: number; }");
        return a;

    } else {
AssertType(b, "{ x: number; z?: number; }");
        return b;
    }
}

// returns typeof b
function f10() {
    if (true) {
AssertType(true, "boolean");

AssertType(b, "{ x: number; z?: number; }");
        return b;

    } else {
AssertType(a, "{ x: number; y?: number; }");
        return a;
    }
}

// returns number => void
function f11() {
    if (true) {
AssertType(true, "boolean");

AssertType((x: number) => { }, "(number) => void");
        return (x: number) => { 

AssertType(x, "number");
}

    } else {
AssertType((x: Object) => { }, "(Object) => void");
        return (x: Object) => { 

AssertType(x, "Object");
}
    }
}

// returns Object => void
function f12() {
    if (true) {
AssertType(true, "boolean");

AssertType((x: Object) => { }, "(Object) => void");
        return (x: Object) => { 

AssertType(x, "Object");
}

    } else {
AssertType((x: number) => { }, "(number) => void");
AssertType(x, "number");
        return (x: number) => { }        
    }
}

