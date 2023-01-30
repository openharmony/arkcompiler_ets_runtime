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

// === tests/cases/conformance/es6/computedProperties/computedPropertyNamesContextualType2_ES5.ts ===
declare function AssertType(value:any, type:string):void;
interface I {
    [s: string]: (x: any) => number; // Doesn't get hit
    [s: number]: (x: string) => number;
}

let o: I = {
AssertType(o, "I");
AssertType({    [+"foo"](y) { return y.length; },    [+"bar"]: y => y.length}, "{ [number]: (string) => number; }");

    [+"foo"](y) { 
AssertType([+"foo"], "(string) => number");
AssertType(+"foo", "number");
AssertType("foo", "string");
AssertType(y, "string");
AssertType(y.length, "number");
return y.length; },

    [+"bar"]: y => y.length
AssertType([+"bar"], "(string) => number");
AssertType(+"bar", "number");
AssertType("bar", "string");
AssertType(y => y.length, "(string) => number");
AssertType(y, "string");
AssertType(y.length, "number");
}

