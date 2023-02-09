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

// === tests/cases/compiler/api.ts ===
declare function AssertType(value:any, type:string):void;
import {excludePrivateKeys1, excludePrivateKeys2} from './internal';
export const dropPrivateProps1 = <Obj>(obj: Obj) => excludePrivateKeys1(obj);
AssertType(dropPrivateProps1, "<Obj>(Obj) => { [K in import("tests/cases/compiler/internal").PublicKeys1<keyof Obj>]: Obj[K]; }");
AssertType(<Obj>(obj: Obj) => excludePrivateKeys1(obj), "<Obj>(Obj) => { [K in import("tests/cases/compiler/internal").PublicKeys1<keyof Obj>]: Obj[K]; }");
AssertType(obj, "Obj");
AssertType(excludePrivateKeys1(obj), "{ [K in import("tests/cases/compiler/internal").PublicKeys1<keyof Obj>]: Obj[K]; }");
AssertType(excludePrivateKeys1, "<Obj>(Obj) => { [K in import("tests/cases/compiler/internal").PublicKeys1<keyof Obj>]: Obj[K]; }");
AssertType(obj, "Obj");

export const dropPrivateProps2 = <Obj>(obj: Obj) => excludePrivateKeys2(obj);
AssertType(dropPrivateProps2, "<Obj>(Obj) => { [K in keyof Obj extends `_${string}` ? never : keyof Obj]: Obj[K]; }");
AssertType(<Obj>(obj: Obj) => excludePrivateKeys2(obj), "<Obj>(Obj) => { [K in keyof Obj extends `_${string}` ? never : keyof Obj]: Obj[K]; }");
AssertType(obj, "Obj");
AssertType(excludePrivateKeys2(obj), "{ [K in keyof Obj extends `_${string}` ? never : keyof Obj]: Obj[K]; }");
AssertType(excludePrivateKeys2, "<Obj>(Obj) => { [K in keyof Obj extends `_${string}` ? never : keyof Obj]: Obj[K]; }");
AssertType(obj, "Obj");


