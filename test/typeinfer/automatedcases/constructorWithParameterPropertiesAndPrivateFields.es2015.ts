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

// === tests/cases/compiler/constructorWithParameterPropertiesAndPrivateFields.es2015.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/48771

class A {
  readonly #privateField: string;

  constructor(arg: { key: string }, public exposedField: number) {
    ({ key: this.#privateField } = arg);
AssertType(({ key: this.#privateField } = arg), "{ key: string; }");
AssertType({ key: this.#privateField } = arg, "{ key: string; }");
AssertType({ key: this.#privateField }, "{ key: string; }");
AssertType(key, "string");
AssertType(this.#privateField, "string");
AssertType(this, "this");
AssertType(arg, "{ key: string; }");
  }

  log() {
    console.log(this.#privateField);
AssertType(console.log(this.#privateField), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(this.#privateField, "string");
AssertType(this, "this");

    console.log(this.exposedField);
AssertType(console.log(this.exposedField), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(this.exposedField, "number");
AssertType(this, "this");
  }
}

class B {
  readonly #privateField: string;

  constructor(arg: { key: string }, public exposedField: number) {
    "prologue";
AssertType("prologue", "string");

    ({ key: this.#privateField } = arg);
AssertType(({ key: this.#privateField } = arg), "{ key: string; }");
AssertType({ key: this.#privateField } = arg, "{ key: string; }");
AssertType({ key: this.#privateField }, "{ key: string; }");
AssertType(key, "string");
AssertType(this.#privateField, "string");
AssertType(this, "this");
AssertType(arg, "{ key: string; }");
  }

  log() {
    console.log(this.#privateField);
AssertType(console.log(this.#privateField), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(this.#privateField, "string");
AssertType(this, "this");

    console.log(this.exposedField);
AssertType(console.log(this.exposedField), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(this.exposedField, "number");
AssertType(this, "this");
  }
}


