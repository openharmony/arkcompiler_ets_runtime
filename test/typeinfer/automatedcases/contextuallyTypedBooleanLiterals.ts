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

// === tests/cases/compiler/contextuallyTypedBooleanLiterals.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #48363

type Box<T> = {
    get: () => T,
    set: (value: T) => void
}

declare function box<T>(value: T): Box<T>;

const bn1 = box(0);  // Box<number>
AssertType(bn1, "Box<number>");
AssertType(box(0), "Box<number>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(0, "int");

const bn2: Box<number> = box(0);  // Ok
AssertType(bn2, "Box<number>");
AssertType(box(0), "Box<number>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(0, "int");

const bb1 = box(false);  // Box<boolean>
AssertType(bb1, "Box<boolean>");
AssertType(box(false), "Box<boolean>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(false, "boolean");

const bb2: Box<boolean> = box(false);  // Error, box<false> not assignable to Box<boolean>
AssertType(bb2, "Box<boolean>");
AssertType(box(false), "Box<boolean>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(false, "boolean");

// Repro from #48150

interface Observable<T>
{
  (): T;
  (value: T): any;
}

declare function observable<T>(value: T): Observable<T>;

const x: Observable<boolean> = observable(false);
AssertType(x, "Observable<boolean>");
AssertType(observable(false), "Observable<boolean>");
AssertType(observable, "<T>(T) => Observable<T>");
AssertType(false, "boolean");


