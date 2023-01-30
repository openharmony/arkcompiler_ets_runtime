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

// === tests/cases/compiler/unionWithIndexSignature.ts ===
declare function AssertType(value:any, type:string):void;
interface NumList {
  kind: 'n';
  [x: number]: number;
}
interface StrList {
  kind: 's';
  [x: number]: string;
}

export function foo<T extends NumList | StrList>(arr: T & (NumList | StrList)) {
  let zz = arr[1];  // Error
AssertType(zz, "union");
AssertType(arr[1], "union");
AssertType(arr, "union");
AssertType(1, "int");
}

// Repro from #38102

export type TypedArray = Int32Array | Uint8Array;

export function isTypedArray(a: {}): a is Int32Array | Uint8Array {
AssertType(a instanceof Int32Array || a instanceof Uint8Array, "boolean");
AssertType(a instanceof Int32Array, "boolean");
AssertType(a, "{}");
AssertType(Int32Array, "Int32ArrayConstructor");
AssertType(a instanceof Uint8Array, "boolean");
AssertType(a, "{}");
AssertType(Uint8Array, "Uint8ArrayConstructor");
  return a instanceof Int32Array || a instanceof Uint8Array;
}

export function flatten<T extends number|TypedArray>(arr: T) {
  if (isTypedArray(arr)) {
AssertType(isTypedArray(arr), "boolean");
AssertType(isTypedArray, "({}) => a is union");
AssertType(arr, "union");

      arr[1];
AssertType(arr[1], "number");
AssertType(arr, "union");
AssertType(1, "int");
  }
}


