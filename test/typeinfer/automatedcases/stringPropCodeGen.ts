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

// === tests/cases/compiler/stringPropCodeGen.ts ===
declare function AssertType(value:any, type:string):void;
let a = {
AssertType(a, "{ foo: () => void; bar: number; }");
AssertType({  "foo" : function() { },  "bar" : 5}, "{ foo: () => void; bar: number; }");

  "foo" : function() { },
AssertType("foo", "() => void");
AssertType(function() { }, "() => void");

  "bar" : 5
AssertType("bar", "number");
AssertType(5, "int");

};



a.foo();
AssertType(a.foo(), "void");
AssertType(a.foo, "() => void");

a.bar.toString();
AssertType(a.bar.toString(), "string");
AssertType(a.bar.toString, "(?number) => string");


