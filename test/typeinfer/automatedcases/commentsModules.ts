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

// === tests/cases/compiler/commentsModules.ts ===
declare function AssertType(value:any, type:string):void;
/** Module comment*/
module m1 {
    /** b's comment*/
    export let b: number;
    /** foo's comment*/
    function foo() {
AssertType(b, "number");
        return b;
    }
    /** m2 comments*/
    export module m2 {
        /** class comment;*/
        export class c {
        };
        /** i*/
        export let i = new c();
    }
    /** exported function*/
    export function fooExport() {
AssertType(foo(), "number");
AssertType(foo, "() => number");
        return foo();
    }

    // shouldn't appear
    export function foo2Export(/**hm*/ a: string) {
    }

    /** foo3Export
     * comment
     */
    export function foo3Export() {
    }

    /** foo4Export
     * comment
     */
    function foo4Export() {
    }
} // trailing comment module
m1.fooExport();
AssertType(m1.fooExport(), "number");
AssertType(m1.fooExport, "() => number");

let mylet = new m1.m2.c();
AssertType(mylet, "m1.m2.c");
AssertType(new m1.m2.c(), "m1.m2.c");
AssertType(m1.m2.c, "typeof m1.m2.c");

/** module comment of m2.m3*/
module m2.m3 {
    /** Exported class comment*/
    export class c {
    }
} /* trailing dotted module comment*/
new m2.m3.c();
AssertType(new m2.m3.c(), "m2.m3.c");
AssertType(m2.m3.c, "typeof m2.m3.c");

/** module comment of m3.m4.m5*/
module m3.m4.m5 {
    /** Exported class comment*/
    export class c {
    }
} // trailing dotted module 2
new m3.m4.m5.c();
AssertType(new m3.m4.m5.c(), "m3.m4.m5.c");
AssertType(m3.m4.m5.c, "typeof m3.m4.m5.c");

/** module comment of m4.m5.m6*/
module m4.m5.m6 {
    export module m7 {
        /** Exported class comment*/
        export class c {
        }
    } /* trailing inner module */ /* multiple comments*/
}
new m4.m5.m6.m7.c();
AssertType(new m4.m5.m6.m7.c(), "m4.m5.m6.m7.c");
AssertType(m4.m5.m6.m7.c, "typeof m4.m5.m6.m7.c");

/** module comment of m5.m6.m7*/
module m5.m6.m7 {
    /** module m8 comment*/
    export module m8 {
        /** Exported class comment*/
        export class c {
        }
    }
}
new m5.m6.m7.m8.c();
AssertType(new m5.m6.m7.m8.c(), "m5.m6.m7.m8.c");
AssertType(m5.m6.m7.m8.c, "typeof m5.m6.m7.m8.c");

module m6.m7 {
    export module m8 {
        /** Exported class comment*/
        export class c {
        }
    }
}
new m6.m7.m8.c();
AssertType(new m6.m7.m8.c(), "m6.m7.m8.c");
AssertType(m6.m7.m8.c, "typeof m6.m7.m8.c");

module m7.m8 {
    /** module m9 comment*/
    export module m9 {
        /** Exported class comment*/
        export class c {
        }

        /** class d */
        class d {
        }

        // class e
        export class e {
        }
    }
}
new m7.m8.m9.c();
AssertType(new m7.m8.m9.c(), "m7.m8.m9.c");
AssertType(m7.m8.m9.c, "typeof m7.m8.m9.c");


