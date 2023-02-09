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

// === tests/cases/compiler/circularContextualMappedType.ts ===
declare function AssertType(value:any, type:string):void;
type Func<T> = () => T;

type Mapped<T> = { [K in keyof T]: Func<T[K]> };

declare function reproduce(options: number): void;
declare function reproduce<T>(options: Mapped<T>): T

reproduce({
AssertType(reproduce, "{ (number): void; <T>(Mapped<T>): T; }");
AssertType({  name:   () => { return 123 }}, "{ name: () => number; }");
AssertType(reproduce({  name:   () => { return 123 }}), "{ name: number; }");

  name:   () => { 
AssertType(name, "() => number");
return 123 

AssertType(() => { return 123 }, "() => number");

AssertType(123, "int");
}

});

reproduce({
AssertType(reproduce, "{ (number): void; <T>(Mapped<T>): T; }");
AssertType({  name() { return 123 }}, "{ name(): number; }");
AssertType(reproduce({  name() { return 123 }}), "{ name: number; }");

  name() { 
AssertType(name, "() => number");
return 123 

AssertType(123, "int");
}

});

reproduce({
AssertType(reproduce, "{ (number): void; <T>(Mapped<T>): T; }");
AssertType({  name: function () { return 123 }}, "{ name: () => number; }");
AssertType(reproduce({  name: function () { return 123 }}), "{ name: number; }");

  name: function () { 
AssertType(name, "() => number");
return 123 

AssertType(function () { return 123 }, "() => number");

AssertType(123, "int");
}

});


