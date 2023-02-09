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

// === tests/cases/compiler/commentsClass.ts ===
declare function AssertType(value:any, type:string):void;
/** This is class c2 without constuctor*/
class c2 {
} // trailing comment1
let i2 = new c2();
AssertType(i2, "c2");
AssertType(new c2(), "c2");
AssertType(c2, "typeof c2");

let i2_c = c2;
AssertType(i2_c, "typeof c2");
AssertType(c2, "typeof c2");

class c3 {
    /** Constructor comment*/
    constructor() {
    } // trailing comment of constructor
} /* trailing comment 2 */
let i3 = new c3();
AssertType(i3, "c3");
AssertType(new c3(), "c3");
AssertType(c3, "typeof c3");

let i3_c = c3;
AssertType(i3_c, "typeof c3");
AssertType(c3, "typeof c3");

/** Class comment*/
class c4 {
    /** Constructor comment*/
    constructor() {
    } /* trailing comment of constructor 2*/
}
let i4 = new c4();
AssertType(i4, "c4");
AssertType(new c4(), "c4");
AssertType(c4, "typeof c4");

let i4_c = c4;
AssertType(i4_c, "typeof c4");
AssertType(c4, "typeof c4");

/** Class with statics*/
class c5 {
    static s1: number;
}
let i5 = new c5();
AssertType(i5, "c5");
AssertType(new c5(), "c5");
AssertType(c5, "typeof c5");

let i5_c = c5;
AssertType(i5_c, "typeof c5");
AssertType(c5, "typeof c5");

/// class with statics and constructor
class c6 { /// class with statics and constructor2
    /// s1 comment
    static s1: number; /// s1 comment2
    /// constructor comment
    constructor() { /// constructor comment2
    }
}
let i6 = new c6();
AssertType(i6, "c6");
AssertType(new c6(), "c6");
AssertType(c6, "typeof c6");

let i6_c = c6;
AssertType(i6_c, "typeof c6");
AssertType(c6, "typeof c6");

// class with statics and constructor
class c7 {
    // s1 comment
    static s1: number;
    // constructor comment
    constructor() {
    }
}
let i7 = new c7();
AssertType(i7, "c7");
AssertType(new c7(), "c7");
AssertType(c7, "typeof c7");

let i7_c = c7;
AssertType(i7_c, "typeof c7");
AssertType(c7, "typeof c7");

/** class with statics and constructor
 */
class c8 {
    /** s1 comment */
    static s1: number; /** s1 comment2 */
    /** constructor comment 
    */
    constructor() {
        /** constructor comment2 
        */
    }
}
let i8 = new c8();
AssertType(i8, "c8");
AssertType(new c8(), "c8");
AssertType(c8, "typeof c8");

let i8_c = c8;
AssertType(i8_c, "typeof c8");
AssertType(c8, "typeof c8");

class c9 {
    constructor() {
        /// This is some detached comment

        // should emit this leading comment of } too
    }
}


