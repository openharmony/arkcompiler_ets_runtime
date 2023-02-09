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

// === tests/cases/compiler/subtypeRelationForNever.ts ===
declare function AssertType(value:any, type:string):void;
function fail(message: string) : never { throw new Error(message); 
AssertType(new Error(message), "Error");

AssertType(Error, "ErrorConstructor");

AssertType(message, "string");
}

function withFew<a, r>(values: a[], haveFew: (values: a[]) => r, haveNone: (reason: string) => r): r {
AssertType(values.length > 0 ? haveFew(values) : haveNone('No values.'), "r");
AssertType(values.length > 0, "boolean");
AssertType(values.length, "number");
AssertType(0, "int");
AssertType(haveFew(values), "r");
AssertType(haveFew, "(a[]) => r");
AssertType(values, "a[]");
AssertType(haveNone('No values.'), "r");
AssertType(haveNone, "(string) => r");
AssertType('No values.', "string");
    return values.length > 0 ? haveFew(values) : haveNone('No values.');
}
function id<a>(value: a) : a { 
AssertType(value, "a");
return value; 
}

const result = withFew([1, 2, 3], id, fail); // expected result is number[]
AssertType(result, "number[]");
AssertType(withFew([1, 2, 3], id, fail), "number[]");
AssertType(withFew, "<a, r>(a[], (a[]) => r, (string) => r) => r");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(id, "<a>(a) => a");
AssertType(fail, "(string) => never");


