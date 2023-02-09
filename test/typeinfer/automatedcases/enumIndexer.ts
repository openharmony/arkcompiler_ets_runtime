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

// === tests/cases/compiler/enumIndexer.ts ===
declare function AssertType(value:any, type:string):void;
enum MyEnumType {
    foo, bar
}
let _arr = [{ key: 'foo' }, { key: 'bar' }]
AssertType(_arr, "{ key: string; }[]");
AssertType([{ key: 'foo' }, { key: 'bar' }], "{ key: string; }[]");
AssertType({ key: 'foo' }, "{ key: string; }");
AssertType(key, "string");
AssertType('foo', "string");
AssertType({ key: 'bar' }, "{ key: string; }");
AssertType(key, "string");
AssertType('bar', "string");

let enumValue = MyEnumType.foo;
AssertType(enumValue, "MyEnumType");
AssertType(MyEnumType.foo, "MyEnumType.foo");

let x = _arr.map(o => MyEnumType[o.key] === enumValue); // these are not same type
AssertType(x, "boolean[]");
AssertType(_arr.map(o => MyEnumType[o.key] === enumValue), "boolean[]");
AssertType(_arr.map, "<U>(({ key: string; }, number, { key: string; }[]) => U, ?any) => U[]");
AssertType(o => MyEnumType[o.key] === enumValue, "({ key: string; }) => boolean");
AssertType(o, "{ key: string; }");
AssertType(MyEnumType[o.key] === enumValue, "boolean");
AssertType(MyEnumType[o.key], "error");
AssertType(MyEnumType, "typeof MyEnumType");
AssertType(o.key, "string");
AssertType(enumValue, "MyEnumType");


