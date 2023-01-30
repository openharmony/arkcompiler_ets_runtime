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

// === tests/cases/compiler/genericTypeWithMultipleBases3.ts ===
declare function AssertType(value:any, type:string):void;
interface IA<T> {

foo(x: T): T;

}

interface IB<T> {

bar(x: T): T;

}

interface IC<T> extends IA<T>, IB<T> { }

let c: IC<number>;
AssertType(c, "IC<number>");

let x = c.foo;
AssertType(x, "(number) => number");
AssertType(c.foo, "(number) => number");

let y = c.bar;
AssertType(y, "(number) => number");
AssertType(c.bar, "(number) => number");


