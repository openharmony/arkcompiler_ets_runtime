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

// === tests/cases/compiler/main.ts ===
declare function AssertType(value:any, type:string):void;
import { c0, c1, s0, T0, T1, T2, T3 } from "./module";
import * as M from "./module";

namespace N {
    export const c2 = "a";
    export const c3 = 1;
    export const s1: typeof s0 = s0;

    export interface T4 {
        [N.c2]: number;
        [N.c3]: string;
        [N.s1]: boolean;
    }
    export declare class T5 implements T4 {
        [N.c2]: number;
        [N.c3]: string;
        [N.s1]: boolean;
    }
    export declare class T6 extends T5 {
    }
    export declare type T7 = {
        [N.c2]: number;
        [N.c3]: string;
        [N.s1]: boolean;
    };
}

export const c4 = "a";
AssertType(c4, "string");
AssertType("a", "string");

export const c5 = 1;
AssertType(c5, "int");
AssertType(1, "int");

export const s2: typeof s0 = s0;
AssertType(s2, "unique symbol");
AssertType(s0, "unique symbol");
AssertType(s0, "unique symbol");

interface T8 {
    [c4]: number;
    [c5]: string;
    [s2]: boolean;
}
declare class T9 implements T8 {
    [c4]: number;
    [c5]: string;
    [s2]: boolean;
}
declare class T10 extends T9 {
}
declare type T11 = {
    [c4]: number;
    [c5]: string;
    [s2]: boolean;
};

interface T12 {
    a: number;
    1: string;
    [s2]: boolean;
}
declare class T13 implements T2 {
    a: number;
    1: string;
    [s2]: boolean;
}
declare class T14 extends T13 {
}
declare type T15 = {
    a: number;
    1: string;
    [s2]: boolean;
};

declare class C {
    static a: number;
    static 1: string;
    static [s2]: boolean;
}

let t0: T0;
AssertType(t0, "T0");

let t1: T1;
AssertType(t1, "T1");

let t2: T2;
AssertType(t2, "T2");

let t3: T3;
AssertType(t3, "T3");

let t0_1: M.T0;
AssertType(t0_1, "T0");
AssertType(M, "any");

let t1_1: M.T1;
AssertType(t1_1, "T1");
AssertType(M, "any");

let t2_1: M.T2;
AssertType(t2_1, "T2");
AssertType(M, "any");

let t3_1: M.T3;
AssertType(t3_1, "T3");
AssertType(M, "any");

let t4: N.T4;
AssertType(t4, "N.T4");
AssertType(N, "any");

let t5: N.T5;
AssertType(t5, "N.T5");
AssertType(N, "any");

let t6: N.T6;
AssertType(t6, "N.T6");
AssertType(N, "any");

let t7: N.T7;
AssertType(t7, "N.T7");
AssertType(N, "any");

let t8: T8;
AssertType(t8, "T8");

let t9: T9;
AssertType(t9, "T9");

let t10: T10;
AssertType(t10, "T10");

let t11: T11;
AssertType(t11, "T11");

let t12: T12;
AssertType(t12, "T12");

let t13: T13;
AssertType(t13, "T13");

let t14: T14;
AssertType(t14, "T14");

let t15: T15;
AssertType(t15, "T15");

// assignability
t0 = t1, t0 = t2, t0 = t3, t1 = t0, t1 = t2, t1 = t3, t2 = t0, t2 = t1, t2 = t3, t3 = t0, t3 = t1, t3 = t2;
AssertType(t0 = t1, t0 = t2, t0 = t3, t1 = t0, t1 = t2, t1 = t3, t2 = t0, t2 = t1, t2 = t3, t3 = t0, t3 = t1, t3 = t2, "T2");
AssertType(t0 = t1, t0 = t2, t0 = t3, t1 = t0, t1 = t2, t1 = t3, t2 = t0, t2 = t1, t2 = t3, t3 = t0, t3 = t1, "T1");
AssertType(t0 = t1, t0 = t2, t0 = t3, t1 = t0, t1 = t2, t1 = t3, t2 = t0, t2 = t1, t2 = t3, t3 = t0, "T0");
AssertType(t0 = t1, t0 = t2, t0 = t3, t1 = t0, t1 = t2, t1 = t3, t2 = t0, t2 = t1, t2 = t3, "T3");
AssertType(t0 = t1, t0 = t2, t0 = t3, t1 = t0, t1 = t2, t1 = t3, t2 = t0, t2 = t1, "T1");
AssertType(t0 = t1, t0 = t2, t0 = t3, t1 = t0, t1 = t2, t1 = t3, t2 = t0, "T0");
AssertType(t0 = t1, t0 = t2, t0 = t3, t1 = t0, t1 = t2, t1 = t3, "T3");
AssertType(t0 = t1, t0 = t2, t0 = t3, t1 = t0, t1 = t2, "T2");
AssertType(t0 = t1, t0 = t2, t0 = t3, t1 = t0, "T0");
AssertType(t0 = t1, t0 = t2, t0 = t3, "T3");
AssertType(t0 = t1, t0 = t2, "T2");
AssertType(t0 = t1, "T1");
AssertType(t0, "T0");
AssertType(t1, "T1");
AssertType(t0 = t2, "T2");
AssertType(t0, "T0");
AssertType(t2, "T2");
AssertType(t0 = t3, "T3");
AssertType(t0, "T0");
AssertType(t3, "T3");
AssertType(t1 = t0, "T0");
AssertType(t1, "T1");
AssertType(t0, "T0");
AssertType(t1 = t2, "T2");
AssertType(t1, "T1");
AssertType(t2, "T2");
AssertType(t1 = t3, "T3");
AssertType(t1, "T1");
AssertType(t3, "T3");
AssertType(t2 = t0, "T0");
AssertType(t2, "T2");
AssertType(t0, "T0");
AssertType(t2 = t1, "T1");
AssertType(t2, "T2");
AssertType(t1, "T1");
AssertType(t2 = t3, "T3");
AssertType(t2, "T2");
AssertType(t3, "T3");
AssertType(t3 = t0, "T0");
AssertType(t3, "T3");
AssertType(t0, "T0");
AssertType(t3 = t1, "T1");
AssertType(t3, "T3");
AssertType(t1, "T1");
AssertType(t3 = t2, "T2");
AssertType(t3, "T3");
AssertType(t2, "T2");

t4 = t5, t4 = t6, t4 = t7, t5 = t4, t5 = t6, t5 = t7, t6 = t4, t6 = t5, t6 = t7, t7 = t4, t7 = t5, t7 = t6;
AssertType(t4 = t5, t4 = t6, t4 = t7, t5 = t4, t5 = t6, t5 = t7, t6 = t4, t6 = t5, t6 = t7, t7 = t4, t7 = t5, t7 = t6, "N.T6");
AssertType(t4 = t5, t4 = t6, t4 = t7, t5 = t4, t5 = t6, t5 = t7, t6 = t4, t6 = t5, t6 = t7, t7 = t4, t7 = t5, "N.T5");
AssertType(t4 = t5, t4 = t6, t4 = t7, t5 = t4, t5 = t6, t5 = t7, t6 = t4, t6 = t5, t6 = t7, t7 = t4, "N.T4");
AssertType(t4 = t5, t4 = t6, t4 = t7, t5 = t4, t5 = t6, t5 = t7, t6 = t4, t6 = t5, t6 = t7, "N.T7");
AssertType(t4 = t5, t4 = t6, t4 = t7, t5 = t4, t5 = t6, t5 = t7, t6 = t4, t6 = t5, "N.T5");
AssertType(t4 = t5, t4 = t6, t4 = t7, t5 = t4, t5 = t6, t5 = t7, t6 = t4, "N.T4");
AssertType(t4 = t5, t4 = t6, t4 = t7, t5 = t4, t5 = t6, t5 = t7, "N.T7");
AssertType(t4 = t5, t4 = t6, t4 = t7, t5 = t4, t5 = t6, "N.T6");
AssertType(t4 = t5, t4 = t6, t4 = t7, t5 = t4, "N.T4");
AssertType(t4 = t5, t4 = t6, t4 = t7, "N.T7");
AssertType(t4 = t5, t4 = t6, "N.T6");
AssertType(t4 = t5, "N.T5");
AssertType(t4, "N.T4");
AssertType(t5, "N.T5");
AssertType(t4 = t6, "N.T6");
AssertType(t4, "N.T4");
AssertType(t6, "N.T6");
AssertType(t4 = t7, "N.T7");
AssertType(t4, "N.T4");
AssertType(t7, "N.T7");
AssertType(t5 = t4, "N.T4");
AssertType(t5, "N.T5");
AssertType(t4, "N.T4");
AssertType(t5 = t6, "N.T6");
AssertType(t5, "N.T5");
AssertType(t6, "N.T6");
AssertType(t5 = t7, "N.T7");
AssertType(t5, "N.T5");
AssertType(t7, "N.T7");
AssertType(t6 = t4, "N.T4");
AssertType(t6, "N.T6");
AssertType(t4, "N.T4");
AssertType(t6 = t5, "N.T5");
AssertType(t6, "N.T6");
AssertType(t5, "N.T5");
AssertType(t6 = t7, "N.T7");
AssertType(t6, "N.T6");
AssertType(t7, "N.T7");
AssertType(t7 = t4, "N.T4");
AssertType(t7, "N.T7");
AssertType(t4, "N.T4");
AssertType(t7 = t5, "N.T5");
AssertType(t7, "N.T7");
AssertType(t5, "N.T5");
AssertType(t7 = t6, "N.T6");
AssertType(t7, "N.T7");
AssertType(t6, "N.T6");

t0 = t12, t0 = t13, t0 = t14, t0 = t15, t12 = t0, t13 = t0, t14 = t0, t15 = t0;
AssertType(t0 = t12, t0 = t13, t0 = t14, t0 = t15, t12 = t0, t13 = t0, t14 = t0, t15 = t0, "T0");
AssertType(t0 = t12, t0 = t13, t0 = t14, t0 = t15, t12 = t0, t13 = t0, t14 = t0, "T0");
AssertType(t0 = t12, t0 = t13, t0 = t14, t0 = t15, t12 = t0, t13 = t0, "T0");
AssertType(t0 = t12, t0 = t13, t0 = t14, t0 = t15, t12 = t0, "T0");
AssertType(t0 = t12, t0 = t13, t0 = t14, t0 = t15, "T15");
AssertType(t0 = t12, t0 = t13, t0 = t14, "T14");
AssertType(t0 = t12, t0 = t13, "T13");
AssertType(t0 = t12, "T12");
AssertType(t0, "T0");
AssertType(t12, "T12");
AssertType(t0 = t13, "T13");
AssertType(t0, "T0");
AssertType(t13, "T13");
AssertType(t0 = t14, "T14");
AssertType(t0, "T0");
AssertType(t14, "T14");
AssertType(t0 = t15, "T15");
AssertType(t0, "T0");
AssertType(t15, "T15");
AssertType(t12 = t0, "T0");
AssertType(t12, "T12");
AssertType(t0, "T0");
AssertType(t13 = t0, "T0");
AssertType(t13, "T13");
AssertType(t0, "T0");
AssertType(t14 = t0, "T0");
AssertType(t14, "T14");
AssertType(t0, "T0");
AssertType(t15 = t0, "T0");
AssertType(t15, "T15");
AssertType(t0, "T0");

t0 = C; // static side
AssertType(t0 = C, "typeof C");
AssertType(t0, "T0");
AssertType(C, "typeof C");

// object literals
export const o1 = {
AssertType(o1, "{ a: number; 1: string; [s0]: boolean; }");
AssertType({    [c4]: 1,    [c5]: "a",    [s2]: true}, "{ a: number; 1: string; [s0]: boolean; }");

    [c4]: 1,
AssertType([c4], "number");
AssertType(c4, "string");
AssertType(1, "int");

    [c5]: "a",
AssertType([c5], "string");
AssertType(c5, "int");
AssertType("a", "string");

    [s2]: true
AssertType([s2], "boolean");
AssertType(s2, "unique symbol");
AssertType(true, "boolean");

};

// check element access types
export const o1_c4 = o1[c4];
AssertType(o1_c4, "number");
AssertType(o1[c4], "number");
AssertType(o1, "{ a: number; 1: string; [s0]: boolean; }");
AssertType(c4, "string");

export const o1_c5 = o1[c5];
AssertType(o1_c5, "string");
AssertType(o1[c5], "string");
AssertType(o1, "{ a: number; 1: string; [s0]: boolean; }");
AssertType(c5, "int");

export const o1_s2 = o1[s2];
AssertType(o1_s2, "boolean");
AssertType(o1[s2], "boolean");
AssertType(o1, "{ a: number; 1: string; [s0]: boolean; }");
AssertType(s2, "unique symbol");

export const o2: T0 = o1;
AssertType(o2, "T0");
AssertType(o1, "{ a: number; 1: string; [s0]: boolean; }");

// recursive declarations
// (type parameter indirection courtesy of #20400)
declare const rI: RI<"a">;
AssertType(rI, "RI<"a">");

rI.x
AssertType(rI.x, "string");

interface RI<T extends "a" | "b"> {
    x: T;
    [rI.x]: "b";
}

declare const rC: RC<"a">;
AssertType(rC, "RC<"a">");

rC.x
AssertType(rC.x, "string");

declare class RC<T extends "a" | "b"> {
    x: T;
    [rC.x]: "b";
}


