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

// === tests/cases/conformance/expressions/functions/contextuallyTypedFunctionExpressionsAndReturnAnnotations.ts ===
declare function AssertType(value:any, type:string):void;
declare function foo(x: (y: string) => (y2: number) => void);

// Contextually type the parameter even if there is a return annotation
foo((y): (y2: number) => void => {
AssertType(foo, "((string) => (number) => void) => any");
AssertType((y): (y2: number) => void => {    let z = y.charAt(0); // Should be string    return null;}, "(string) => (number) => void");
AssertType(y, "string");
AssertType(y2, "number");
AssertType(foo((y): (y2: number) => void => {    let z = y.charAt(0); // Should be string    return null;}), "any");

    let z = y.charAt(0); // Should be string
AssertType(z, "string");
AssertType(y.charAt(0), "string");
AssertType(y.charAt, "(number) => string");
AssertType(0, "int");

AssertType(null, "null");
    return null;

});

foo((y: string) => {
AssertType(foo, "((string) => (number) => void) => any");
AssertType((y: string) => {    return y2 => {        let z = y2.toFixed(); // Should be string        return 0;    };}, "(string) => (number) => number");
AssertType(y, "string");
AssertType(foo((y: string) => {    return y2 => {        let z = y2.toFixed(); // Should be string        return 0;    };}), "any");

AssertType(y2 => {        let z = y2.toFixed(); // Should be string        return 0;    }, "(number) => number");
AssertType(y2, "number");
    return y2 => {

        let z = y2.toFixed(); // Should be string
AssertType(z, "string");
AssertType(y2.toFixed(), "string");
AssertType(y2.toFixed, "(?number) => string");

AssertType(0, "int");
        return 0;

    };
});

