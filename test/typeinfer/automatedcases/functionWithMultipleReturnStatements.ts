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

// === tests/cases/conformance/types/typeRelationships/bestCommonType/functionWithMultipleReturnStatements.ts ===
declare function AssertType(value:any, type:string):void;
// return type of a function with multiple returns is the BCT of each return statement
// it is an error if there is no single BCT, these are error cases

function f1() {
    if (true) {
AssertType(true, "boolean");

AssertType(1, "int");
        return 1;

    } else {
AssertType('', "string");
        return '';
    }
}

function f2() {
    if (true) {
AssertType(true, "boolean");

AssertType(1, "int");
        return 1;

    } else if (false) {
AssertType(false, "boolean");

AssertType(2, "int");
        return 2;

    } else {
AssertType('', "string");
        return '';
    }
}

function f3() {
    try {
AssertType(1, "int");
        return 1;
    }
    catch (e) {
AssertType(e, "any");

AssertType('', "string");
        return '';
    }
}

function f4() {
    try {
AssertType(1, "int");
        return 1;
    }
    catch (e) {
AssertType(e, "any");

    }
    finally {
AssertType('', "string");
        return '';
    }
}

function f5() {
AssertType(1, "int");
    return 1;

AssertType('', "string");
    return '';
}

function f6<T, U>(x: T, y:U) {
    if (true) {
AssertType(true, "boolean");

AssertType(x, "T");
        return x;

    } else {
AssertType(y, "U");
        return y;
    }
}

function f8<T extends U, U extends V, V>(x: T, y: U) {
    if (true) {
AssertType(true, "boolean");

AssertType(x, "T");
        return x;

    } else {
AssertType(y, "U");
        return y;
    }
}


