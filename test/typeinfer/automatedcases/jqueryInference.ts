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

// === tests/cases/compiler/jqueryInference.ts ===
declare function AssertType(value:any, type:string):void;
// #22362
interface MyPromise<T, U> {
    then(cb: (t: T) => void): void;
    thenUnion(cb: (t: T | U) => void): this;
}

interface DoNothingAlias<T, U> extends MyPromise<T, U> { }

declare function shouldBeIdentity<T, U>(p: DoNothingAlias<T, U>): MyPromise<T, U>;

declare const p1: MyPromise<boolean, any>;
AssertType(p1, "MyPromise<boolean, any>");

let p2 = shouldBeIdentity(p1);
AssertType(p2, "MyPromise<boolean, any>");
AssertType(shouldBeIdentity(p1), "MyPromise<boolean, any>");
AssertType(shouldBeIdentity, "<T, U>(DoNothingAlias<T, U>) => MyPromise<T, U>");
AssertType(p1, "MyPromise<boolean, any>");

let p2: MyPromise<boolean, any>;
AssertType(p2, "MyPromise<boolean, any>");


