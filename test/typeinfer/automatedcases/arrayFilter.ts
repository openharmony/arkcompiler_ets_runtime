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

// === tests/cases/compiler/arrayFilter.ts ===
declare function AssertType(value:any, type:string):void;
let foo = [
AssertType(foo, "{ name: string; }[]");
AssertType([    { name: 'bar' },    { name: null },    { name: 'baz' }], "{ name: string; }[]");

    { name: 'bar' },
AssertType({ name: 'bar' }, "{ name: string; }");
AssertType(name, "string");
AssertType('bar', "string");

    { name: null },
AssertType({ name: null }, "{ name: null; }");
AssertType(name, "null");
AssertType(null, "null");

    { name: 'baz' 
AssertType({ name: 'baz' }, "{ name: string; }");

AssertType(name, "string");

AssertType('baz', "string");
}

]

foo.filter(x => x.name); //should accepted all possible types not only boolean! 
AssertType(foo.filter(x => x.name), "{ name: string; }[]");
AssertType(foo.filter, "{ <S extends { name: string; }>(({ name: string; }, number, { name: string; }[]) => value is S, ?any): S[]; (({ name: string; }, number, { name: string; }[]) => unknown, ?any): { name: string; }[]; }");
AssertType(x => x.name, "({ name: string; }) => string");
AssertType(x, "{ name: string; }");
AssertType(x.name, "string");


