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

// === tests/cases/compiler/genericCallbacksAndClassHierarchy.ts ===
declare function AssertType(value:any, type:string):void;
module M {
    export interface I<T> {
        subscribe(callback: (newValue: T) => void ): any;
    }
    export class C1<T> {
        public value: I<T>;
    }
    export class A<T> {
        public dummy: any;
    }
    export class B<T> extends C1<A<T>> { }
    export class D<T> {
        _subscribe(viewModel: B<T>): void {
            let f = (newValue: A<T>) => { };
AssertType(f, "(A<T>) => void");
AssertType((newValue: A<T>) => { }, "(A<T>) => void");
AssertType(newValue, "A<T>");

            let v: I<A<T>> = viewModel.value;
AssertType(v, "I<A<T>>");
AssertType(viewModel.value, "I<A<T>>");

            // both of these should work
            v.subscribe(f);
AssertType(v.subscribe(f), "any");
AssertType(v.subscribe, "((A<T>) => void) => any");
AssertType(f, "(A<T>) => void");

            v.subscribe((newValue: A<T>) => { });
AssertType(v.subscribe((newValue: A<T>) => { }), "any");
AssertType(v.subscribe, "((A<T>) => void) => any");
AssertType((newValue: A<T>) => { }, "(A<T>) => void");
AssertType(newValue, "A<T>");
        }
    }
}

