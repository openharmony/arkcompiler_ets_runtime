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

// === tests/cases/conformance/types/conditional/variance.ts ===
declare function AssertType(value:any, type:string):void;
// Test cases for parameter variances affected by conditional types.

// Repro from #30047

interface Foo<T> {
  prop: T extends unknown ? true : false;
}

const foo = { prop: true } as const;
AssertType(foo, "{ readonly prop: true; }");
AssertType({ prop: true } as const, "{ readonly prop: true; }");
AssertType({ prop: true }, "{ readonly prop: true; }");
AssertType(prop, "boolean");
AssertType(true, "boolean");

const x: Foo<1> = foo;
AssertType(x, "Foo<1>");
AssertType(foo, "{ readonly prop: true; }");

const y: Foo<number> = foo;
AssertType(y, "Foo<number>");
AssertType(foo, "{ readonly prop: true; }");

const z: Foo<number> = x;
AssertType(z, "Foo<number>");
AssertType(x, "Foo<1>");


// Repro from #30118

class Bar<T extends string> {
  private static instance: Bar<string>[] = [];

  cast(_name: ([T] extends [string] ? string : string)) { }
  
  pushThis() {
    Bar.instance.push(this);
AssertType(Bar.instance.push(this), "number");
AssertType(Bar.instance.push, "(...Bar<string>[]) => number");
AssertType(Bar.instance, "Bar<string>[]");
AssertType(this, "this");
  }
}


