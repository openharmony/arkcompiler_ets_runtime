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

// === tests/cases/compiler/lateBoundConstraintTypeChecksCorrectly.ts ===
declare function AssertType(value:any, type:string):void;
declare const fooProp: unique symbol;
AssertType(fooProp, "unique symbol");

declare const barProp: unique symbol;
AssertType(barProp, "unique symbol");

type BothProps = typeof fooProp | typeof barProp;

export interface Foo<T> {
  [fooProp]: T;
  [barProp]: string;
}

function f<T extends Foo<number>>(x: T) {
    const abc = x[fooProp]; // expected: 'T[typeof fooProp]'
AssertType(abc, "number");
AssertType(x[fooProp], "number");
AssertType(x, "T");
AssertType(fooProp, "unique symbol");

    /**
     * Expected: no error
     */
    const def: T[typeof fooProp] = x[fooProp];
AssertType(def, "T[unique symbol]");
AssertType(fooProp, "unique symbol");
AssertType(x[fooProp], "number");
AssertType(x, "T");
AssertType(fooProp, "unique symbol");

    const def2: T[typeof barProp] = x[barProp];
AssertType(def2, "T[unique symbol]");
AssertType(barProp, "unique symbol");
AssertType(x[barProp], "string");
AssertType(x, "T");
AssertType(barProp, "unique symbol");
}


