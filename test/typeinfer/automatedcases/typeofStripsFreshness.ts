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

// === tests/cases/compiler/typeofStripsFreshness.ts ===
declare function AssertType(value:any, type:string):void;
interface Collection<T> {
    elems: T[];
}
interface CollectionStatic {
    new <T>(): Collection<T>;
}
declare const Collection: CollectionStatic;
AssertType(Collection, "CollectionStatic");

const ALL = "all";
AssertType(ALL, "string");
AssertType("all", "string");

type All = typeof ALL;

const result: Collection<All> = new Collection();
AssertType(result, "Collection<"all">");
AssertType(new Collection(), "Collection<"all">");
AssertType(Collection, "CollectionStatic");

const ANOTHER = "another";
AssertType(ANOTHER, "string");
AssertType("another", "string");

type Another = typeof ANOTHER;

type Both = Another | All;

const result2: Collection<Both> = new Collection();
AssertType(result2, "Collection<Both>");
AssertType(new Collection(), "Collection<Both>");
AssertType(Collection, "CollectionStatic");


