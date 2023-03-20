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

// === tests/cases/compiler/privacyImport.ts ===
declare function AssertType(value:any, type:string):void;
export module m1 {
    export module m1_M1_public {
        export class c1 {
        }
        export function f1() {
AssertType(new c1, "c1");
AssertType(c1, "typeof c1");
            return new c1;
        }
        export let v1 = c1;
        export let v2: c1;
    }

    module m1_M2_private {
        export class c1 {
        }
        export function f1() {
AssertType(new c1, "c1");
AssertType(c1, "typeof c1");
            return new c1;
        }
        export let v1 = c1;
        export let v2: c1;
    }

    import m1_im1_private = m1_M1_public;
    export let m1_im1_private_v1_public = m1_im1_private.c1;
    export let m1_im1_private_v2_public = new m1_im1_private.c1();
    export let m1_im1_private_v3_public = m1_im1_private.f1;
    export let m1_im1_private_v4_public = m1_im1_private.f1();
    let m1_im1_private_v1_private = m1_im1_private.c1;
    let m1_im1_private_v2_private = new m1_im1_private.c1();
    let m1_im1_private_v3_private = m1_im1_private.f1;
    let m1_im1_private_v4_private = m1_im1_private.f1();


    import m1_im2_private = m1_M2_private;
    export let m1_im2_private_v1_public = m1_im2_private.c1;
    export let m1_im2_private_v2_public = new m1_im2_private.c1();
    export let m1_im2_private_v3_public = m1_im2_private.f1;
    export let m1_im2_private_v4_public = m1_im2_private.f1();
    let m1_im2_private_v1_private = m1_im2_private.c1;
    let m1_im2_private_v2_private = new m1_im2_private.c1();
    let m1_im2_private_v3_private = m1_im2_private.f1;
    let m1_im2_private_v4_private = m1_im2_private.f1();

    export import m1_im1_public = m1_M1_public;
    export import m1_im2_public = m1_M2_private;
}

module m2 {
    export module m2_M1_public {
        export class c1 {
        }
        export function f1() {
AssertType(new c1, "c1");
AssertType(c1, "typeof c1");
            return new c1;
        }
        export let v1 = c1;
        export let v2: c1;
    }

    module m2_M2_private {
        export class c1 {
        }
        export function f1() {
AssertType(new c1, "c1");
AssertType(c1, "typeof c1");
            return new c1;
        }
        export let v1 = c1;
        export let v2: c1;
    }

    import m1_im1_private = m2_M1_public;
    export let m1_im1_private_v1_public = m1_im1_private.c1;
    export let m1_im1_private_v2_public = new m1_im1_private.c1();
    export let m1_im1_private_v3_public = m1_im1_private.f1;
    export let m1_im1_private_v4_public = m1_im1_private.f1();
    let m1_im1_private_v1_private = m1_im1_private.c1;
    let m1_im1_private_v2_private = new m1_im1_private.c1();
    let m1_im1_private_v3_private = m1_im1_private.f1;
    let m1_im1_private_v4_private = m1_im1_private.f1();


    import m1_im2_private = m2_M2_private;
    export let m1_im2_private_v1_public = m1_im2_private.c1;
    export let m1_im2_private_v2_public = new m1_im2_private.c1();
    export let m1_im2_private_v3_public = m1_im2_private.f1;
    export let m1_im2_private_v4_public = m1_im2_private.f1();
    let m1_im2_private_v1_private = m1_im2_private.c1;
    let m1_im2_private_v2_private = new m1_im2_private.c1();
    let m1_im2_private_v3_private = m1_im2_private.f1;
    let m1_im2_private_v4_private = m1_im2_private.f1();

    export import m1_im1_public = m2_M1_public;
    export import m1_im2_public = m2_M2_private;
}

export module glo_M1_public {
    export class c1 {
    }
    export function f1() {
AssertType(new c1, "c1");
AssertType(c1, "typeof c1");
        return new c1;
    }
    export let v1 = c1;
    export let v2: c1;
}

export module glo_M3_private {
    export class c1 {
    }
    export function f1() {
AssertType(new c1, "c1");
AssertType(c1, "typeof c1");
        return new c1;
    }
    export let v1 = c1;
    export let v2: c1;
}

import glo_im1_private = glo_M1_public;
export let glo_im1_private_v1_public = glo_im1_private.c1;
AssertType(glo_im1_private_v1_public, "typeof glo_im1_private.c1");
AssertType(glo_im1_private.c1, "typeof glo_im1_private.c1");

export let glo_im1_private_v2_public = new glo_im1_private.c1();
AssertType(glo_im1_private_v2_public, "glo_im1_private.c1");
AssertType(new glo_im1_private.c1(), "glo_im1_private.c1");
AssertType(glo_im1_private.c1, "typeof glo_im1_private.c1");

export let glo_im1_private_v3_public = glo_im1_private.f1;
AssertType(glo_im1_private_v3_public, "() => glo_im1_private.c1");
AssertType(glo_im1_private.f1, "() => glo_im1_private.c1");

export let glo_im1_private_v4_public = glo_im1_private.f1();
AssertType(glo_im1_private_v4_public, "glo_im1_private.c1");
AssertType(glo_im1_private.f1(), "glo_im1_private.c1");
AssertType(glo_im1_private.f1, "() => glo_im1_private.c1");

let glo_im1_private_v1_private = glo_im1_private.c1;
AssertType(glo_im1_private_v1_private, "typeof glo_im1_private.c1");
AssertType(glo_im1_private.c1, "typeof glo_im1_private.c1");

let glo_im1_private_v2_private = new glo_im1_private.c1();
AssertType(glo_im1_private_v2_private, "glo_im1_private.c1");
AssertType(new glo_im1_private.c1(), "glo_im1_private.c1");
AssertType(glo_im1_private.c1, "typeof glo_im1_private.c1");

let glo_im1_private_v3_private = glo_im1_private.f1;
AssertType(glo_im1_private_v3_private, "() => glo_im1_private.c1");
AssertType(glo_im1_private.f1, "() => glo_im1_private.c1");

let glo_im1_private_v4_private = glo_im1_private.f1();
AssertType(glo_im1_private_v4_private, "glo_im1_private.c1");
AssertType(glo_im1_private.f1(), "glo_im1_private.c1");
AssertType(glo_im1_private.f1, "() => glo_im1_private.c1");

import glo_im3_private = glo_M3_private;
export let glo_im3_private_v1_public = glo_im3_private.c1;
AssertType(glo_im3_private_v1_public, "typeof glo_im3_private.c1");
AssertType(glo_im3_private.c1, "typeof glo_im3_private.c1");

export let glo_im3_private_v2_public = new glo_im3_private.c1();
AssertType(glo_im3_private_v2_public, "glo_im3_private.c1");
AssertType(new glo_im3_private.c1(), "glo_im3_private.c1");
AssertType(glo_im3_private.c1, "typeof glo_im3_private.c1");

export let glo_im3_private_v3_public = glo_im3_private.f1;
AssertType(glo_im3_private_v3_public, "() => glo_im3_private.c1");
AssertType(glo_im3_private.f1, "() => glo_im3_private.c1");

export let glo_im3_private_v4_public = glo_im3_private.f1();
AssertType(glo_im3_private_v4_public, "glo_im3_private.c1");
AssertType(glo_im3_private.f1(), "glo_im3_private.c1");
AssertType(glo_im3_private.f1, "() => glo_im3_private.c1");

let glo_im3_private_v1_private = glo_im3_private.c1;
AssertType(glo_im3_private_v1_private, "typeof glo_im3_private.c1");
AssertType(glo_im3_private.c1, "typeof glo_im3_private.c1");

let glo_im3_private_v2_private = new glo_im3_private.c1();
AssertType(glo_im3_private_v2_private, "glo_im3_private.c1");
AssertType(new glo_im3_private.c1(), "glo_im3_private.c1");
AssertType(glo_im3_private.c1, "typeof glo_im3_private.c1");

let glo_im3_private_v3_private = glo_im3_private.f1;
AssertType(glo_im3_private_v3_private, "() => glo_im3_private.c1");
AssertType(glo_im3_private.f1, "() => glo_im3_private.c1");

let glo_im3_private_v4_private = glo_im3_private.f1();
AssertType(glo_im3_private_v4_private, "glo_im3_private.c1");
AssertType(glo_im3_private.f1(), "glo_im3_private.c1");
AssertType(glo_im3_private.f1, "() => glo_im3_private.c1");

module m2 {
    module m4 {
        let a = 10;
    }

}

export module m3 {
    module m4 {
        let a = 10;
    }

}

