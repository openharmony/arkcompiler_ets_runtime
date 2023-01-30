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

// === tests/cases/conformance/classes/members/constructorFunctionTypes/constructorHasPrototypeProperty.ts ===
declare function AssertType(value:any, type:string):void;

module NonGeneric {
    class C {
        foo: string;
    }

    class D extends C {
        bar: string;
    }

    let r = C.prototype;
    r.foo;
    let r2 = D.prototype;
    r2.bar;
}

module Generic {
    class C<T,U> {
        foo: T;
        bar: U;
    }

    class D<T,U> extends C<T,U> {
        baz: T;
        bing: U;
    }

    let r = C.prototype; // C<any, any>
    let ra = r.foo; // any
    let r2 = D.prototype; // D<any, any>
    let rb = r2.baz; // any
}

