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

// === tests/cases/compiler/typePartameterConstraintInstantiatedWithDefaultWhenCheckingDefault.ts ===
declare function AssertType(value:any, type:string):void;
// tricky interface
interface Settable<T, V> {
    set(value: V): T;
}

// implement
class Identity<V> implements Settable<Identity<V>, V> {
    readonly item: V;
    constructor(value: V) {
        this.item = value;
AssertType(this.item = value, "V");
AssertType(this.item, "V");
AssertType(this, "this");
AssertType(value, "V");
    }
    public set(value: V): Identity<V> {
AssertType(new Identity<V>(value), "Identity<V>");
AssertType(Identity, "typeof Identity");
AssertType(value, "V");
        return new Identity<V>(value);
    }
}

// generic parameter default
interface Test1<V, T extends Settable<T, V> = Identity<V>> { };
let test1: Test1<number>;
AssertType(test1, "Test1<number, Identity<number>>");

// not generic parameter default
interface Test2Base<V, T extends Settable<T, V>> { };
type Test2<V> = Test2Base<V, Identity<V>>;
let test2: Test2<number>;
AssertType(test2, "Test2<number>");


