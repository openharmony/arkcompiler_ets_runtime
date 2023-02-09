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

// === tests/cases/compiler/infinitelyExpandingTypes4.ts ===
declare function AssertType(value:any, type:string):void;
interface Query<T> {
    // ...
    groupBy<K>(keySelector: (item: T) => K): Query<Grouping<K, T>>;
    // ...
}

interface QueryEnumerator<T> {
    // ...
    groupBy<K>(keySelector: (item: T) => K): QueryEnumerator<Grouping<K, T>>;
    // ...
}

interface Grouping<K, T> extends Query<T> {
    key(): K;
}

let q1: Query<number>;
AssertType(q1, "Query<number>");

let q2: QueryEnumerator<number>;
AssertType(q2, "QueryEnumerator<number>");

let q3: Query<number>;
AssertType(q3, "Query<number>");

q1 = q2; // should error
AssertType(q1 = q2, "QueryEnumerator<number>");
AssertType(q1, "Query<number>");
AssertType(q2, "QueryEnumerator<number>");

q1 = q3; // should not error
AssertType(q1 = q3, "Query<number>");
AssertType(q1, "Query<number>");
AssertType(q3, "Query<number>");


