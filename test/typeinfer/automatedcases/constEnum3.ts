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

// === tests/cases/conformance/constEnums/constEnum3.ts ===
declare function AssertType(value:any, type:string):void;
const enum TestType { foo, bar }
type TestTypeStr = keyof typeof TestType;

function f1(f: TestType) { }
function f2(f: TestTypeStr) { }

f1(TestType.foo)
AssertType(f1(TestType.foo), "void");
AssertType(f1, "(TestType) => void");
AssertType(TestType.foo, "TestType.foo");

f1(TestType.bar)
AssertType(f1(TestType.bar), "void");
AssertType(f1, "(TestType) => void");
AssertType(TestType.bar, "TestType.bar");

f2('foo')
AssertType(f2('foo'), "void");
AssertType(f2, "(union) => void");
AssertType('foo', "string");

f2('bar')
AssertType(f2('bar'), "void");
AssertType(f2, "(union) => void");
AssertType('bar', "string");


