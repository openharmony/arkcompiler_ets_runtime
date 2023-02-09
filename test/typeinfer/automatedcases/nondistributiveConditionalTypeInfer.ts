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

// === tests/cases/compiler/nondistributiveConditionalTypeInfer.ts ===
declare function AssertType(value:any, type:string):void;

type _R<T> = [T] extends [{ _R: (_: infer R) => void }] ? R : never;
type _E<T> = [T] extends [{ _E: () => infer E }] ? E : never;
type _A<T> = [T] extends [{ _A: () => infer A }] ? A : never;

interface Sync<R, E, A> {
  _R: (_: R) => void;
  _E: () => E;
  _A: () => A;
}

type R = _R<Sync<number, string, void>>;
type E = _E<Sync<number, string, void>>;
type A = _A<Sync<number, string, void>>;

