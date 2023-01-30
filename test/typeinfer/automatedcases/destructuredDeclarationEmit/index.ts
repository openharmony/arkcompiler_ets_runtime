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

// === tests/cases/compiler/index.ts ===
declare function AssertType(value:any, type:string):void;
import { foo, arr } from './foo';
export { foo, arr };

const { bar: baz, bat, bam: { bork: { bar: ibar, baz: ibaz } } } = foo;
AssertType(bar, "any");
AssertType(baz, "string");
AssertType(bat, "string");
AssertType(bam, "any");
AssertType(bork, "any");
AssertType(bar, "any");
AssertType(ibar, "string");
AssertType(baz, "any");
AssertType(ibaz, "string");
AssertType(foo, "{ bar: string; bat: string; bam: { bork: { bar: string; baz: string; }; }; }");

export { baz, ibaz };

const [ , one, , [, bee, , [, {sec} ]]] = arr;
AssertType(, "undefined");
AssertType(one, "int");
AssertType(, "undefined");
AssertType(, "undefined");
AssertType(bee, "string");
AssertType(, "undefined");
AssertType(, "undefined");
AssertType(sec, "string");
AssertType(arr, "[0, 1, 2, ["a", "b", "c", [{ def: "def"; }, { sec: "sec"; }]]]");

export { one, bee, sec };

const getFoo = () => ({
AssertType(getFoo, "() => { foo: string; }");
AssertType(() => ({    foo: 'foo'}), "() => { foo: string; }");
AssertType(({    foo: 'foo'}), "{ foo: string; }");
AssertType({    foo: 'foo'}, "{ foo: string; }");

    foo: 'foo'
AssertType(foo, "string");
AssertType('foo', "string");

});

const { foo: foo2 } = getFoo();
AssertType(foo, "any");
AssertType(foo2, "string");
AssertType(getFoo(), "{ foo: string; }");
AssertType(getFoo, "() => { foo: string; }");

export { foo2 };


