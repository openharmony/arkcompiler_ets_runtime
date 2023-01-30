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

// === tests/cases/conformance/types/mapped/mappedTypes3.ts ===
declare function AssertType(value:any, type:string):void;
class Box<P> {
    value: P;
}

type Boxified<T> = {
    [K in keyof T]: Box<T[K]>;
}

declare function boxify<T>(obj: T): Boxified<T>;
declare function unboxify<T>(obj: Boxified<T>): T;

interface Bacon {
    isPerfect: boolean;
    weight: number;
}

interface BoxifiedBacon {
    isPerfect: Box<boolean>;
    weight: Box<number>;
}

function f1(b: Bacon) {
    let bb = boxify(b);
AssertType(bb, "Boxified<Bacon>");
AssertType(boxify(b), "Boxified<Bacon>");
AssertType(boxify, "<T>(T) => Boxified<T>");
AssertType(b, "Bacon");

    let isPerfect = bb.isPerfect.value;
AssertType(isPerfect, "boolean");
AssertType(bb.isPerfect.value, "boolean");
AssertType(bb.isPerfect, "Box<boolean>");

    let weight = bb.weight.value;
AssertType(weight, "number");
AssertType(bb.weight.value, "number");
AssertType(bb.weight, "Box<number>");
}

function f2(bb: Boxified<Bacon>) {
    let b = unboxify(bb);  // Infer Bacon for T
AssertType(b, "Bacon");
AssertType(unboxify(bb), "Bacon");
AssertType(unboxify, "<T>(Boxified<T>) => T");
AssertType(bb, "Boxified<Bacon>");

    let bool = b.isPerfect;
AssertType(bool, "boolean");
AssertType(b.isPerfect, "boolean");

    let weight = b.weight;
AssertType(weight, "number");
AssertType(b.weight, "number");
}

function f3(bb: BoxifiedBacon) {
    let b = unboxify<Bacon>(bb);  // Explicit type parameter required
AssertType(b, "Bacon");
AssertType(unboxify<Bacon>(bb), "Bacon");
AssertType(unboxify, "<T>(Boxified<T>) => T");
AssertType(bb, "BoxifiedBacon");

    let bool = b.isPerfect;
AssertType(bool, "boolean");
AssertType(b.isPerfect, "boolean");

    let weight = bb.weight;
AssertType(weight, "Box<number>");
AssertType(bb.weight, "Box<number>");
}

