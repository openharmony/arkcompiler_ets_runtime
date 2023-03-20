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

// === tests/cases/compiler/privacyGloImport.ts ===
declare function AssertType(value:any, type:string):void;
module m1 {
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

module glo_M1_public {
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

declare module "glo_M2_public" {
    export function f1();
    export class c1 {
    }
    export let v1: { new (): c1; };
    export let v2: c1;
}

declare module "use_glo_M1_public" {
    import use_glo_M1_public = glo_M1_public;
    export let use_glo_M1_public_v1_public: { new (): use_glo_M1_public.c1; };
    export let use_glo_M1_public_v2_public: typeof use_glo_M1_public;
    export let use_glo_M1_public_v3_public: ()=> use_glo_M1_public.c1;
    let use_glo_M1_public_v1_private: { new (): use_glo_M1_public.c1; };
    let use_glo_M1_public_v2_private: typeof use_glo_M1_public;
    let use_glo_M1_public_v3_private: () => use_glo_M1_public.c1;

    import * as use_glo_M2_public from "glo_M2_public";
    export let use_glo_M2_public_v1_public: { new (): use_glo_M2_public.c1; };
    export let use_glo_M2_public_v2_public: typeof use_glo_M2_public;
    export let use_glo_M2_public_v3_public: () => use_glo_M2_public.c1;
    let use_glo_M2_public_v1_private: { new (): use_glo_M2_public.c1; };
    let use_glo_M2_public_v2_private: typeof use_glo_M2_public;
    let use_glo_M2_public_v3_private: () => use_glo_M2_public.c1;

    module m2 {
        import nonerrorImport = glo_M1_public;

        module m5 {
            import m5_nonerrorImport = glo_M1_public;
        }
    }
}

declare module "anotherParseError" {
    module m2 {
        //declare module "abc" {
        //}
    }

    module m2 {
        //module "abc2" {
        //}
    }
    //module "abc3" {
    //}
}

module m2 {
    module m4 {
        let a = 10;
    }

}

