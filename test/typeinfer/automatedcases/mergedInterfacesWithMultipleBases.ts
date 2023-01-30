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

// === tests/cases/conformance/interfaces/declarationMerging/mergedInterfacesWithMultipleBases.ts ===
declare function AssertType(value:any, type:string):void;
// merged interfaces behave as if all extends clauses from each declaration are merged together
// no errors expected

class C {
    a: number;
}

class C2 {
    b: number;
}

interface A extends C {
    y: string;
}

interface A extends C2 {
    z: string;
}

class D implements A {
    a: number;
    b: number;
    y: string;
    z: string;
}

let a: A;
AssertType(a, "A");

let r = a.a;
AssertType(r, "number");
AssertType(a.a, "number");

// generic interfaces in a module
module M {
    class C<T> {
        a: T;
    }

    class C2<T> {
        b: T;
    }

    interface A<T> extends C<T> {
        y: T;
    }

    interface A<T> extends C2<string> {
        z: T;
    }

    class D implements A<boolean> {
        a: boolean;
        b: string;
        y: boolean;
        z: boolean;
    }
}

