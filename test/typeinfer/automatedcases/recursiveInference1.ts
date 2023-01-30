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

// === tests/cases/compiler/recursiveInference1.ts ===
declare function AssertType(value:any, type:string):void;
function fib(x:number) { 
AssertType(x <= 1 ? x : fib(x - 1) + fib(x - 2), "any");
return x <= 1 ? x : fib(x - 1) + fib(x - 2); 

AssertType(x <= 1, "boolean");

AssertType(x, "number");

AssertType(1, "int");

AssertType(x, "number");

AssertType(fib(x - 1) + fib(x - 2), "any");

AssertType(fib(x - 1), "any");

AssertType(fib, "(number) => any");

AssertType(x - 1, "number");

AssertType(x, "number");

AssertType(1, "int");

AssertType(fib(x - 2), "any");

AssertType(fib, "(number) => any");

AssertType(x - 2, "number");

AssertType(x, "number");

AssertType(2, "int");
}

let result = fib(5);
AssertType(result, "any");
AssertType(fib(5), "any");
AssertType(fib, "(number) => any");
AssertType(5, "int");


