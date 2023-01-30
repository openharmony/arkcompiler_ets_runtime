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

// === tests/cases/conformance/classes/classDeclarations/classHeritageSpecification/derivedTypeDoesNotRequireExtendsClause.ts ===
declare function AssertType(value:any, type:string):void;
class Base {
    foo: string;
}

class Derived {
    foo: string;
    bar: number;
}

class Derived2 extends Base {
    bar: string;
}

let b: Base;
AssertType(b, "Base");

let d1: Derived;
AssertType(d1, "Derived");

let d2: Derived2;
AssertType(d2, "Derived2");

b = d1;
AssertType(b = d1, "Derived");
AssertType(b, "Base");
AssertType(d1, "Derived");

b = d2;
AssertType(b = d2, "Derived2");
AssertType(b, "Base");
AssertType(d2, "Derived2");

let r: Base[] = [d1, d2];
AssertType(r, "Base[]");
AssertType([d1, d2], "(union)[]");
AssertType(d1, "Derived");
AssertType(d2, "Derived2");


