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

// === tests/cases/compiler/test.ts ===
declare function AssertType(value:any, type:string):void;
import {dropPrivateProps1, dropPrivateProps2} from './api';
const a = dropPrivateProps1({foo: 42, _bar: 'secret'}); // type is {foo: number
AssertType(a, "{ foo: number; }");

AssertType(dropPrivateProps1({foo: 42, _bar: 'secret'}), "{ foo: number; }");

AssertType(dropPrivateProps1, "<Obj>(Obj) => { [K in import("tests/cases/compiler/internal").PublicKeys1<keyof Obj>]: Obj[K]; }");

AssertType({foo: 42, _bar: 'secret'}, "{ foo: number; _bar: string; }");

AssertType(foo, "number");

AssertType(42, "int");

AssertType(_bar, "string");

AssertType('secret', "string");
}

//a._bar                                                // error: _bar does not exist           <===== as expected
const b = dropPrivateProps2({foo: 42, _bar: 'secret'}); // type is {foo: number, _bar: string
AssertType(b, "{ foo: number; }");

AssertType(dropPrivateProps2({foo: 42, _bar: 'secret'}), "{ foo: number; }");

AssertType(dropPrivateProps2, "<Obj>(Obj) => { [K in keyof Obj extends `_${string}` ? never : keyof Obj]: Obj[K]; }");

AssertType({foo: 42, _bar: 'secret'}, "{ foo: number; _bar: string; }");

AssertType(foo, "number");

AssertType(42, "int");

AssertType(_bar, "string");

AssertType('secret', "string");
}

//b._bar                                                // no error, type of b._bar is string   <===== NOT expected


