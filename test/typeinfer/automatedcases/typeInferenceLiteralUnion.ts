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

// === tests/cases/compiler/typeInferenceLiteralUnion.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #10901
/**
 * Administrivia: JavaScript primitive types and Date
 */
export type Primitive = number | string | boolean | Date;

/**
 * Administrivia: anything with a valueOf(): number method is comparable, so we allow it in numeric operations
 */
interface Numeric {
    valueOf(): number;
}

// Not very useful, but meets Numeric
class NumCoercible {
    public a: number;

    constructor(a: number) {
        this.a = a;
AssertType(this.a = a, "number");
AssertType(this.a, "number");
AssertType(this, "this");
AssertType(a, "number");
    }
    public valueOf() {
AssertType(this.a, "number");
AssertType(this, "this");
        return this.a;
    }
}

/**
 * Return the min and max simultaneously.
 */
export function extent<T extends Numeric>(array: Array<T | Primitive>): [T | Primitive, T | Primitive] | [undefined, undefined] {
AssertType([undefined, undefined], "[undefined, undefined]");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");
    return [undefined, undefined];
}


let extentMixed: [Primitive | NumCoercible, Primitive | NumCoercible] | [undefined, undefined];
AssertType(extentMixed, "union");

extentMixed = extent([new NumCoercible(10), 13, '12', true]);
AssertType(extentMixed = extent([new NumCoercible(10), 13, '12', true]), "union");
AssertType(extentMixed, "union");
AssertType(extent([new NumCoercible(10), 13, '12', true]), "union");
AssertType(extent, "<T extends Numeric>((union)[]) => union");
AssertType([new NumCoercible(10), 13, '12', true], "(union)[]");
AssertType(new NumCoercible(10), "NumCoercible");
AssertType(NumCoercible, "typeof NumCoercible");
AssertType(10, "int");
AssertType(13, "int");
AssertType('12', "string");
AssertType(true, "boolean");


