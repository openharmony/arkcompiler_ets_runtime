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

// === tests/cases/compiler/declFileTypeofInAnonymousType.ts ===
declare function AssertType(value:any, type:string):void;
module m1 {
    export class c {
    }
    export enum e {
        weekday,
        weekend,
        holiday
    }
}
let a: { c: m1.c; };
AssertType(a, "{ c: m1.c; }");
AssertType(c, "m1.c");
AssertType(m1, "any");

let b = {
AssertType(b, "{ c: typeof m1.c; m1: typeof m1; }");
AssertType({    c: m1.c,    m1: m1}, "{ c: typeof m1.c; m1: typeof m1; }");

    c: m1.c,
AssertType(c, "typeof m1.c");
AssertType(m1.c, "typeof m1.c");

    m1: m1
AssertType(m1, "typeof m1");
AssertType(m1, "typeof m1");

};
let c = { m1: m1 };
AssertType(c, "{ m1: typeof m1; }");
AssertType({ m1: m1 }, "{ m1: typeof m1; }");
AssertType(m1, "typeof m1");
AssertType(m1, "typeof m1");

let d = {
AssertType(d, "{ m: { mod: typeof m1; }; mc: { cl: typeof m1.c; }; me: { en: typeof m1.e; }; mh: m1.e; }");
AssertType({    m: { mod: m1 },    mc: { cl: m1.c },    me: { en: m1.e },    mh: m1.e.holiday}, "{ m: { mod: typeof m1; }; mc: { cl: typeof m1.c; }; me: { en: typeof m1.e; }; mh: m1.e; }");

    m: { mod: m1 },
AssertType(m, "{ mod: typeof m1; }");
AssertType({ mod: m1 }, "{ mod: typeof m1; }");
AssertType(mod, "typeof m1");
AssertType(m1, "typeof m1");

    mc: { cl: m1.c },
AssertType(mc, "{ cl: typeof m1.c; }");
AssertType({ cl: m1.c }, "{ cl: typeof m1.c; }");
AssertType(cl, "typeof m1.c");
AssertType(m1.c, "typeof m1.c");

    me: { en: m1.e },
AssertType(me, "{ en: typeof m1.e; }");
AssertType({ en: m1.e }, "{ en: typeof m1.e; }");
AssertType(en, "typeof m1.e");
AssertType(m1.e, "typeof m1.e");

    mh: m1.e.holiday
AssertType(mh, "m1.e");
AssertType(m1.e.holiday, "m1.e.holiday");

};

