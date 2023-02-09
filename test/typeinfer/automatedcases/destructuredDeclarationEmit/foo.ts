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

// === tests/cases/compiler/foo.ts ===
declare function AssertType(value:any, type:string):void;
const foo = { bar: 'hello', bat: 'world', bam: { bork: { bar: 'a', baz: 'b' } } };
AssertType(foo, "{ bar: string; bat: string; bam: { bork: { bar: string; baz: string; }; }; }");
AssertType({ bar: 'hello', bat: 'world', bam: { bork: { bar: 'a', baz: 'b' } } }, "{ bar: string; bat: string; bam: { bork: { bar: string; baz: string; }; }; }");
AssertType(bar, "string");
AssertType('hello', "string");
AssertType(bat, "string");
AssertType('world', "string");
AssertType(bam, "{ bork: { bar: string; baz: string; }; }");
AssertType({ bork: { bar: 'a', baz: 'b' } }, "{ bork: { bar: string; baz: string; }; }");
AssertType(bork, "{ bar: string; baz: string; }");
AssertType({ bar: 'a', baz: 'b' }, "{ bar: string; baz: string; }");
AssertType(bar, "string");
AssertType('a', "string");
AssertType(baz, "string");
AssertType('b', "string");

const arr: [0, 1, 2, ['a', 'b', 'c', [{def: 'def'}, {sec: 'sec'}]]] = [0, 1, 2, ['a', 'b', 'c', [{def: 'def'}, {sec: 'sec'}]]];
AssertType(arr, "[0, 1, 2, ["a", "b", "c", [{ def: 'def'; }, { sec: 'sec'; }]]]");
AssertType(def, "string");
AssertType(sec, "string");
AssertType([0, 1, 2, ['a', 'b', 'c', [{def: 'def'}, {sec: 'sec'}]]], "[0, 1, 2, ["a", "b", "c", [{ def: "def"; }, { sec: "sec"; }]]]");
AssertType(0, "int");
AssertType(1, "int");
AssertType(2, "int");
AssertType(['a', 'b', 'c', [{def: 'def'}, {sec: 'sec'}]], "["a", "b", "c", [{ def: "def"; }, { sec: "sec"; }]]");
AssertType('a', "string");
AssertType('b', "string");
AssertType('c', "string");
AssertType([{def: 'def'}, {sec: 'sec'}], "[{ def: "def"; }, { sec: "sec"; }]");
AssertType({def: 'def'}, "{ def: "def"; }");
AssertType(def, "string");
AssertType('def', "string");
AssertType({sec: 'sec'}, "{ sec: "sec"; }");
AssertType(sec, "string");
AssertType('sec', "string");

export { foo, arr };

