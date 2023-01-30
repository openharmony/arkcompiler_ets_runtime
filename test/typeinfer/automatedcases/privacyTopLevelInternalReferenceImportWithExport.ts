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

// === tests/cases/compiler/privacyTopLevelInternalReferenceImportWithExport.ts ===
declare function AssertType(value:any, type:string):void;
// private elements
module m_private {
    export class c_private {
    }
    export enum e_private {
        Happy,
        Grumpy
    }
    export function f_private() {
AssertType(new c_private(), "c_private");
AssertType(c_private, "typeof c_private");
        return new c_private();
    }
    export let v_private = new c_private();
    export interface i_private {
    }
    export module mi_private {
        export class c {
        }
    }
    export module mu_private {
        export interface i {
        }
    }
}

// Public elements
export module m_public {
    export class c_public {
    }
    export enum e_public {
        Happy,
        Grumpy
    }
    export function f_public() {
AssertType(new c_public(), "c_public");
AssertType(c_public, "typeof c_public");
        return new c_public();
    }
    export let v_public = 10;
    export interface i_public {
    }
    export module mi_public {
        export class c {
        }
    }
    export module mu_public {
        export interface i {
        }
    }
}

// Privacy errors - importing private elements
export import im_public_c_private = m_private.c_private;
export import im_public_e_private = m_private.e_private;
export import im_public_f_private = m_private.f_private;
export import im_public_v_private = m_private.v_private;
export import im_public_i_private = m_private.i_private;
export import im_public_mi_private = m_private.mi_private;
export import im_public_mu_private = m_private.mu_private;

// Usage of privacy error imports
let privateUse_im_public_c_private = new im_public_c_private();
AssertType(privateUse_im_public_c_private, "im_public_c_private");
AssertType(new im_public_c_private(), "im_public_c_private");
AssertType(im_public_c_private, "typeof im_public_c_private");

export let publicUse_im_public_c_private = new im_public_c_private();
AssertType(publicUse_im_public_c_private, "im_public_c_private");
AssertType(new im_public_c_private(), "im_public_c_private");
AssertType(im_public_c_private, "typeof im_public_c_private");

let privateUse_im_public_e_private = im_public_e_private.Happy;
AssertType(privateUse_im_public_e_private, "im_public_e_private");
AssertType(im_public_e_private.Happy, "im_public_e_private.Happy");

export let publicUse_im_public_e_private = im_public_e_private.Grumpy;
AssertType(publicUse_im_public_e_private, "im_public_e_private");
AssertType(im_public_e_private.Grumpy, "im_public_e_private.Grumpy");

let privateUse_im_public_f_private = im_public_f_private();
AssertType(privateUse_im_public_f_private, "im_public_c_private");
AssertType(im_public_f_private(), "im_public_c_private");
AssertType(im_public_f_private, "() => im_public_c_private");

export let publicUse_im_public_f_private = im_public_f_private();
AssertType(publicUse_im_public_f_private, "im_public_c_private");
AssertType(im_public_f_private(), "im_public_c_private");
AssertType(im_public_f_private, "() => im_public_c_private");

let privateUse_im_public_v_private = im_public_v_private;
AssertType(privateUse_im_public_v_private, "im_public_c_private");
AssertType(im_public_v_private, "im_public_c_private");

export let publicUse_im_public_v_private = im_public_v_private;
AssertType(publicUse_im_public_v_private, "im_public_c_private");
AssertType(im_public_v_private, "im_public_c_private");

let privateUse_im_public_i_private: im_public_i_private;
AssertType(privateUse_im_public_i_private, "im_public_i_private");

export let publicUse_im_public_i_private: im_public_i_private;
AssertType(publicUse_im_public_i_private, "im_public_i_private");

let privateUse_im_public_mi_private = new im_public_mi_private.c();
AssertType(privateUse_im_public_mi_private, "im_public_mi_private.c");
AssertType(new im_public_mi_private.c(), "im_public_mi_private.c");
AssertType(im_public_mi_private.c, "typeof im_public_mi_private.c");

export let publicUse_im_public_mi_private = new im_public_mi_private.c();
AssertType(publicUse_im_public_mi_private, "im_public_mi_private.c");
AssertType(new im_public_mi_private.c(), "im_public_mi_private.c");
AssertType(im_public_mi_private.c, "typeof im_public_mi_private.c");

let privateUse_im_public_mu_private: im_public_mu_private.i;
AssertType(privateUse_im_public_mu_private, "im_public_mu_private.i");
AssertType(im_public_mu_private, "any");

export let publicUse_im_public_mu_private: im_public_mu_private.i;
AssertType(publicUse_im_public_mu_private, "im_public_mu_private.i");
AssertType(im_public_mu_private, "any");


// No Privacy errors - importing public elements
export import im_public_c_public = m_public.c_public;
export import im_public_e_public = m_public.e_public;
export import im_public_f_public = m_public.f_public;
export import im_public_v_public = m_public.v_public;
export import im_public_i_public = m_public.i_public;
export import im_public_mi_public = m_public.mi_public;
export import im_public_mu_public = m_public.mu_public;

// Usage of above decls
let privateUse_im_public_c_public = new im_public_c_public();
AssertType(privateUse_im_public_c_public, "im_public_c_public");
AssertType(new im_public_c_public(), "im_public_c_public");
AssertType(im_public_c_public, "typeof im_public_c_public");

export let publicUse_im_public_c_public = new im_public_c_public();
AssertType(publicUse_im_public_c_public, "im_public_c_public");
AssertType(new im_public_c_public(), "im_public_c_public");
AssertType(im_public_c_public, "typeof im_public_c_public");

let privateUse_im_public_e_public = im_public_e_public.Happy;
AssertType(privateUse_im_public_e_public, "im_public_e_public");
AssertType(im_public_e_public.Happy, "im_public_e_public.Happy");

export let publicUse_im_public_e_public = im_public_e_public.Grumpy;
AssertType(publicUse_im_public_e_public, "im_public_e_public");
AssertType(im_public_e_public.Grumpy, "im_public_e_public.Grumpy");

let privateUse_im_public_f_public = im_public_f_public();
AssertType(privateUse_im_public_f_public, "im_public_c_public");
AssertType(im_public_f_public(), "im_public_c_public");
AssertType(im_public_f_public, "() => im_public_c_public");

export let publicUse_im_public_f_public = im_public_f_public();
AssertType(publicUse_im_public_f_public, "im_public_c_public");
AssertType(im_public_f_public(), "im_public_c_public");
AssertType(im_public_f_public, "() => im_public_c_public");

let privateUse_im_public_v_public = im_public_v_public;
AssertType(privateUse_im_public_v_public, "number");
AssertType(im_public_v_public, "number");

export let publicUse_im_public_v_public = im_public_v_public;
AssertType(publicUse_im_public_v_public, "number");
AssertType(im_public_v_public, "number");

let privateUse_im_public_i_public: im_public_i_public;
AssertType(privateUse_im_public_i_public, "im_public_i_public");

export let publicUse_im_public_i_public: im_public_i_public;
AssertType(publicUse_im_public_i_public, "im_public_i_public");

let privateUse_im_public_mi_public = new im_public_mi_public.c();
AssertType(privateUse_im_public_mi_public, "im_public_mi_public.c");
AssertType(new im_public_mi_public.c(), "im_public_mi_public.c");
AssertType(im_public_mi_public.c, "typeof im_public_mi_public.c");

export let publicUse_im_public_mi_public = new im_public_mi_public.c();
AssertType(publicUse_im_public_mi_public, "im_public_mi_public.c");
AssertType(new im_public_mi_public.c(), "im_public_mi_public.c");
AssertType(im_public_mi_public.c, "typeof im_public_mi_public.c");

let privateUse_im_public_mu_public: im_public_mu_public.i;
AssertType(privateUse_im_public_mu_public, "im_public_mu_public.i");
AssertType(im_public_mu_public, "any");

export let publicUse_im_public_mu_public: im_public_mu_public.i;
AssertType(publicUse_im_public_mu_public, "im_public_mu_public.i");
AssertType(im_public_mu_public, "any");


