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

// === tests/cases/conformance/controlFlow/controlFlowAliasingCatchVariables.ts ===
declare function AssertType(value:any, type:string):void;
try {}
catch (e) {
    const isString = typeof e === 'string';
AssertType(isString, "boolean");
AssertType(typeof e === 'string', "boolean");
AssertType(typeof e, "union");
AssertType(e, "any");
AssertType('string', "string");

    if (isString) {
AssertType(isString, "boolean");

        e.toUpperCase(); // e string
AssertType(e.toUpperCase(), "string");
AssertType(e.toUpperCase, "() => string");
    }

    if (typeof e === 'string') {
AssertType(typeof e === 'string', "boolean");
AssertType(typeof e, "union");
AssertType(e, "any");
AssertType('string', "string");

        e.toUpperCase(); // e string
AssertType(e.toUpperCase(), "string");
AssertType(e.toUpperCase, "() => string");
    }
}

try {}
catch (e) {
    const isString = typeof e === 'string';
AssertType(isString, "boolean");
AssertType(typeof e === 'string', "boolean");
AssertType(typeof e, "union");
AssertType(e, "any");
AssertType('string', "string");

    e = 1;
AssertType(e = 1, "int");
AssertType(e, "any");
AssertType(1, "int");

    if (isString) {
AssertType(isString, "boolean");

        e.toUpperCase(); // e any/unknown
AssertType(e.toUpperCase(), "any");
AssertType(e.toUpperCase, "any");
    }

    if (typeof e === 'string') {
AssertType(typeof e === 'string', "boolean");
AssertType(typeof e, "union");
AssertType(e, "any");
AssertType('string', "string");

        e.toUpperCase(); // e string
AssertType(e.toUpperCase(), "string");
AssertType(e.toUpperCase, "() => string");
    }
}


