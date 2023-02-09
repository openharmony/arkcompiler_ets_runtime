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

// === tests/cases/conformance/es6/Symbols/symbolProperty20.ts ===
declare function AssertType(value:any, type:string):void;
interface I {
    [Symbol.iterator]: (s: string) => string;
    [Symbol.toStringTag](s: number): number;
}

let i: I = {
AssertType(i, "I");
AssertType({    [Symbol.iterator]: s => s,    [Symbol.toStringTag](n) { return n; }}, "{ [Symbol.iterator]: (string) => string; [Symbol.toStringTag](number): number; }");

    [Symbol.iterator]: s => s,
AssertType([Symbol.iterator], "(string) => string");
AssertType(Symbol.iterator, "unique symbol");
AssertType(s => s, "(string) => string");
AssertType(s, "string");
AssertType(s, "string");

    [Symbol.toStringTag](n) { 
AssertType([Symbol.toStringTag], "(number) => number");
return n; 

AssertType(Symbol.toStringTag, "unique symbol");

AssertType(n, "number");

AssertType(n, "number");
}
}

