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

// === tests/cases/conformance/classes/classStaticBlock/classStaticBlock11.ts ===
declare function AssertType(value:any, type:string):void;
let getX;
AssertType(getX, "any");

class C {
  #x = 1
  constructor(x: number) {
    this.#x = x;
AssertType(this.#x = x, "number");
AssertType(this.#x, "number");
AssertType(this, "this");
AssertType(x, "number");
  }

  static {
    // getX has privileged access to #x
    getX = (obj: C) => obj.#x;
AssertType(getX = (obj: C) => obj.#x, "(C) => number");
AssertType(getX, "any");
AssertType((obj: C) => obj.#x, "(C) => number");
AssertType(obj, "C");
AssertType(obj.#x, "number");
  }
}


