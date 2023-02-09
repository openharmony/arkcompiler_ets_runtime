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

// === tests/cases/conformance/types/any/anyPropertyAccess.ts ===
declare function AssertType(value:any, type:string):void;
let x: any;
AssertType(x, "any");

let a = x.foo;
AssertType(a, "any");
AssertType(x.foo, "any");

let b = x['foo'];
AssertType(b, "any");
AssertType(x['foo'], "any");
AssertType(x, "any");
AssertType('foo', "string");

let c = x['fn']();
AssertType(c, "any");
AssertType(x['fn'](), "any");
AssertType(x['fn'], "any");
AssertType(x, "any");
AssertType('fn', "string");

let d = x.bar.baz;
AssertType(d, "any");
AssertType(x.bar.baz, "any");

let e = x[0].foo;
AssertType(e, "any");
AssertType(x[0].foo, "any");

let f = x['0'].bar;
AssertType(f, "any");
AssertType(x['0'].bar, "any");


