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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericClassWithObjectTypeArgsAndConstraints.ts ===
declare function AssertType(value:any, type:string):void;
// Generic call with constraints infering type parameter from object member properties
// No errors expected

class C {
    x: string;
}

class D {
    x: string;
    y: string;
}

class X<T> {
    x: T;
}

module Class {
    class G<T extends { x: string }> {
        foo<T extends { x: string }>(t: X<T>, t2: X<T>) {
            let x: T;
AssertType(x, "T");

AssertType(x, "T");
            return x;
        }
    }

    let c1 = new X<C>();
    let d1 = new X<D>();
    let g: G<{ x: string; y: string }>;
    let r = g.foo(c1, d1);
    let r2 = g.foo(c1, c1);

    class G2<T extends C> {
        foo2<T extends C>(t: X<T>, t2: X<T>) {
            let x: T;
AssertType(x, "T");

AssertType(x, "T");
            return x;
        }
    }
    let g2: G2<D>;
    let r = g2.foo2(c1, d1);
    let r2 = g2.foo2(c1, c1);
}

module Interface {
    interface G<T extends { x: string }> {
        foo<T extends { x: string }>(t: X<T>, t2: X<T>): T;
    }

    let c1 = new X<C>();
    let d1 = new X<D>();
    let g: G<{ x: string; y: string }>;
    let r = g.foo(c1, d1);
    let r2 = g.foo(c1, c1);

    interface G2<T extends C> {
        foo2<T extends C>(t: X<T>, t2: X<T>): T;
    }

    let g2: G2<D>;
    let r = g2.foo2(c1, d1);
    let r2 = g2.foo2(c1, c1);
}

