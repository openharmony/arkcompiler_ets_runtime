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

// === tests/cases/compiler/genericClassesInModule2.ts ===
declare function AssertType(value:any, type:string):void;
export class A<T1>{
    constructor( public callback: (self: A<T1>) => void) {
        let child = new B(this);
AssertType(child, "B<this>");
AssertType(new B(this), "B<this>");
AssertType(B, "typeof B");
AssertType(this, "this");
    }
    AAA( callback: (self: A<T1>) => void) {
        let child = new B(this);
AssertType(child, "B<this>");
AssertType(new B(this), "B<this>");
AssertType(B, "typeof B");
AssertType(this, "this");
    }
}

export interface C<T1>{
    child: B<T1>;
    (self: C<T1>): void;
    new(callback: (self: C<T1>) => void)
}

export class B<T2> {
    constructor(public parent: T2) { }
}



