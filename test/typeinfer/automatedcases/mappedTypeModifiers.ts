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

// === tests/cases/conformance/types/mapped/mappedTypeModifiers.ts ===
declare function AssertType(value:any, type:string):void;
type T = { a: number, b: string };
type TP = { a?: number, b?: string };
type TR = { readonly a: number, readonly b: string };
type TPR = { readonly a?: number, readonly b?: string };

let v00: "a" | "b";
AssertType(v00, "union");

let v00: keyof T;
AssertType(v00, "union");

let v00: keyof TP;
AssertType(v00, "union");

let v00: keyof TR;
AssertType(v00, "union");

let v00: keyof TPR;
AssertType(v00, "union");

let v01: T;
AssertType(v01, "T");

let v01: { [P in keyof T]: T[P] };
AssertType(v01, "T");

let v01: Pick<T, keyof T>;
AssertType(v01, "T");

let v01: Pick<Pick<T, keyof T>, keyof T>;
AssertType(v01, "T");

let v02: TP;
AssertType(v02, "TP");

let v02: { [P in keyof T]?: T[P] };
AssertType(v02, "TP");

let v02: Partial<T>;
AssertType(v02, "TP");

let v02: { [P in keyof TP]: TP[P] 
AssertType(v02, "TP");
}

let v02: Pick<TP, keyof TP>;
AssertType(v02, "TP");

let v03: TR;
AssertType(v03, "TR");

let v03: { readonly [P in keyof T]: T[P] };
AssertType(v03, "TR");

let v03: Readonly<T>;
AssertType(v03, "TR");

let v03: { [P in keyof TR]: TR[P] 
AssertType(v03, "TR");
}

let v03: Pick<TR, keyof TR>;
AssertType(v03, "TR");

let v04: TPR;
AssertType(v04, "TPR");

let v04: { readonly [P in keyof T]?: T[P] };
AssertType(v04, "TPR");

let v04: Partial<TR>;
AssertType(v04, "TPR");

let v04: Readonly<TP>;
AssertType(v04, "TPR");

let v04: Partial<Readonly<T>>;
AssertType(v04, "TPR");

let v04: Readonly<Partial<T>>;
AssertType(v04, "TPR");

let v04: { [P in keyof TPR]: TPR[P] 
AssertType(v04, "TPR");
}

let v04: Pick<TPR, keyof T>;
AssertType(v04, "TPR");

type Boxified<T> = { [P in keyof T]: { x: T[P] } };

type B = { a: { x: number }, b: { x: string } };
type BP = { a?: { x: number }, b?: { x: string } };
type BR = { readonly a: { x: number }, readonly b: { x: string } };
type BPR = { readonly a?: { x: number }, readonly b?: { x: string } };

let b00: "a" | "b";
AssertType(b00, "union");

let b00: keyof B;
AssertType(b00, "union");

let b00: keyof BP;
AssertType(b00, "union");

let b00: keyof BR;
AssertType(b00, "union");

let b00: keyof BPR;
AssertType(b00, "union");

let b01: B;
AssertType(b01, "B");

let b01: { [P in keyof B]: B[P] };
AssertType(b01, "B");

let b01: Pick<B, keyof B>;
AssertType(b01, "B");

let b01: Pick<Pick<B, keyof B>, keyof B>;
AssertType(b01, "B");

let b02: BP;
AssertType(b02, "BP");

let b02: { [P in keyof B]?: B[P] };
AssertType(b02, "BP");

let b02: Partial<B>;
AssertType(b02, "BP");

let b02: { [P in keyof BP]: BP[P] 
AssertType(b02, "BP");
}

let b02: Pick<BP, keyof BP>;
AssertType(b02, "BP");

let b03: BR;
AssertType(b03, "BR");

let b03: { readonly [P in keyof B]: B[P] };
AssertType(b03, "BR");

let b03: Readonly<B>;
AssertType(b03, "BR");

let b03: { [P in keyof BR]: BR[P] 
AssertType(b03, "BR");
}

let b03: Pick<BR, keyof BR>;
AssertType(b03, "BR");

let b04: BPR;
AssertType(b04, "BPR");

let b04: { readonly [P in keyof B]?: B[P] };
AssertType(b04, "BPR");

let b04: Partial<BR>;
AssertType(b04, "BPR");

let b04: Readonly<BP>;
AssertType(b04, "BPR");

let b04: Partial<Readonly<B>>;
AssertType(b04, "BPR");

let b04: Readonly<Partial<B>>;
AssertType(b04, "BPR");

let b04: { [P in keyof BPR]: BPR[P] 
AssertType(b04, "BPR");
}

let b04: Pick<BPR, keyof BPR>;
AssertType(b04, "BPR");

type Foo = { prop: number, [x: string]: number };

function f1(x: Partial<Foo>) {
    x.prop; // ok
AssertType(x.prop, "union");

    (x["other"] || 0).toFixed();
AssertType((x["other"] || 0).toFixed(), "string");
AssertType((x["other"] || 0).toFixed, "(?union) => string");
AssertType((x["other"] || 0), "number");
AssertType(x["other"] || 0, "number");
AssertType(x["other"], "union");
AssertType(x, "Partial<Foo>");
AssertType("other", "string");
AssertType(0, "int");
}

function f2(x: Readonly<Foo>) {
    x.prop; // ok
AssertType(x.prop, "number");

    x["other"].toFixed();
AssertType(x["other"].toFixed(), "string");
AssertType(x["other"].toFixed, "(?union) => string");
AssertType(x["other"], "number");
AssertType(x, "Readonly<Foo>");
AssertType("other", "string");
}

function f3(x: Boxified<Foo>) {
    x.prop; // ok
AssertType(x.prop, "{ x: number; }");

    x["other"].x.toFixed();
AssertType(x["other"].x.toFixed(), "string");
AssertType(x["other"].x.toFixed, "(?union) => string");
AssertType(x["other"].x, "number");
AssertType(x["other"], "{ x: number; }");
AssertType(x, "Boxified<Foo>");
AssertType("other", "string");
}

function f4(x: { [P in keyof Foo]: Foo[P] }) {
    x.prop; // ok
AssertType(x.prop, "number");

    x["other"].toFixed();
AssertType(x["other"].toFixed(), "string");
AssertType(x["other"].toFixed, "(?union) => string");
AssertType(x["other"], "number");
AssertType(x, "{ [string]: number; prop: number; }");
AssertType("other", "string");
}


