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

// === tests/cases/compiler/genericTypeWithCallableMembers.ts ===
declare function AssertType(value:any, type:string):void;
interface Constructable {
    new (): Constructable;
}
 
class C<T extends Constructable> {
    constructor(public data: T, public data2: Constructable) { }
    create() {
        let x = new this.data(); // no error
AssertType(x, "Constructable");
AssertType(new this.data(), "Constructable");
AssertType(this.data, "T");
AssertType(this, "this");

        let x2 = new this.data2(); // was error, shouldn't be
AssertType(x2, "Constructable");
AssertType(new this.data2(), "Constructable");
AssertType(this.data2, "Constructable");
AssertType(this, "this");
    }
}


