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

// === tests/cases/conformance/types/intersection/intersectionOfUnionNarrowing.ts ===
declare function AssertType(value:any, type:string):void;
interface X {
  a?: { aProp: string };
  b?: { bProp: string };
}
type AorB = { a: object; b: undefined } | { a: undefined; b: object };

declare const q: X & AorB;
AssertType(q, "X & AorB");

if (q.a !== undefined) {
  q.a.aProp;
AssertType(q.a.aProp, "string");
AssertType(q.a, "{ aProp: string; } & object");

} else {
  // q.b is previously incorrectly inferred as potentially undefined
  q.b.bProp;
AssertType(q.b.bProp, "string");
AssertType(q.b, "{ bProp: string; } & object");
}


