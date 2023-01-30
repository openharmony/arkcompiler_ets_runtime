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

// === tests/cases/compiler/recursiveTypeComparison.ts ===
declare function AssertType(value:any, type:string):void;
// Before fix this would take an exceeding long time to complete (#1170)

interface Observable<T> {
    // This member can't be of type T, Property<T>, or Observable<anything but T>
    needThisOne: Observable<T>;
    // Add more to make it slower
    expo1: Property<T[]>; //  0.31 seconds in check
    expo2: Property<T[]>; //  3.11 seconds
    expo3: Property<T[]>; // 82.28 seconds
}
interface Property<T> extends Observable<T> { }

let p: Observable<{}>;
AssertType(p, "Observable<{}>");

let stuck: Property<number> = p;
AssertType(stuck, "Property<number>");
AssertType(p, "Observable<{}>");


