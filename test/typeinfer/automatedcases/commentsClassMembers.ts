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

// === tests/cases/compiler/commentsClassMembers.ts ===
declare function AssertType(value:any, type:string):void;
/** This is comment for c1*/
class c1 {
    /** p1 is property of c1*/
    public p1: number;
    /** sum with property*/
    public p2(/** number to add*/b: number) {
AssertType(this.p1 + b, "number");
AssertType(this.p1, "number");
AssertType(this, "this");
AssertType(b, "number");
        return this.p1 + b;

    } /* trailing comment of method*/
    /** getter property*/
    public get p3() {
AssertType(this.p2(this.p1), "number");
AssertType(this.p2, "(number) => number");
AssertType(this, "this");
AssertType(this.p1, "number");
AssertType(this, "this");
        return this.p2(this.p1);

    }// trailing comment Getter
    /** setter property*/
    public set p3(/** this is value*/value: number) {
        this.p1 = this.p2(value);
AssertType(this.p1 = this.p2(value), "number");
AssertType(this.p1, "number");
AssertType(this, "this");
AssertType(this.p2(value), "number");
AssertType(this.p2, "(number) => number");
AssertType(this, "this");
AssertType(value, "number");

    }// trailing comment Setter
    /** pp1 is property of c1*/
    private pp1: number;
    /** sum with property*/
    private pp2(/** number to add*/b: number) {
AssertType(this.p1 + b, "number");
AssertType(this.p1, "number");
AssertType(this, "this");
AssertType(b, "number");
        return this.p1 + b;

    } // trailing comment of method
    /** getter property*/
    private get pp3() {
AssertType(this.pp2(this.pp1), "number");
AssertType(this.pp2, "(number) => number");
AssertType(this, "this");
AssertType(this.pp1, "number");
AssertType(this, "this");
        return this.pp2(this.pp1);
    }
    /** setter property*/
    private set pp3( /** this is value*/value: number) {
        this.pp1 = this.pp2(value);
AssertType(this.pp1 = this.pp2(value), "number");
AssertType(this.pp1, "number");
AssertType(this, "this");
AssertType(this.pp2(value), "number");
AssertType(this.pp2, "(number) => number");
AssertType(this, "this");
AssertType(value, "number");
    }
    /** Constructor method*/
    constructor() {
    }
    /** s1 is static property of c1*/
    static s1: number;
    /** static sum with property*/
    static s2(/** number to add*/b: number) {
AssertType(c1.s1 + b, "number");
AssertType(c1.s1, "number");
AssertType(b, "number");
        return c1.s1 + b;
    }
    /** static getter property*/
    static get s3() {
AssertType(c1.s2(c1.s1), "number");
AssertType(c1.s2, "(number) => number");
AssertType(c1.s1, "number");
        return c1.s2(c1.s1);

    } /*trailing comment 1 getter*/
    /** setter property*/
    static set s3( /** this is value*/value: number) {
        c1.s1 = c1.s2(value);
AssertType(c1.s1 = c1.s2(value), "number");
AssertType(c1.s1, "number");
AssertType(c1.s2(value), "number");
AssertType(c1.s2, "(number) => number");
AssertType(value, "number");

    }/*trailing comment 2 */ /*setter*/
    public nc_p1: number;
    public nc_p2(b: number) {
AssertType(this.nc_p1 + b, "number");
AssertType(this.nc_p1, "number");
AssertType(this, "this");
AssertType(b, "number");
        return this.nc_p1 + b;
    }
    public get nc_p3() {
AssertType(this.nc_p2(this.nc_p1), "number");
AssertType(this.nc_p2, "(number) => number");
AssertType(this, "this");
AssertType(this.nc_p1, "number");
AssertType(this, "this");
        return this.nc_p2(this.nc_p1);
    }
    public set nc_p3(value: number) {
        this.nc_p1 = this.nc_p2(value);
AssertType(this.nc_p1 = this.nc_p2(value), "number");
AssertType(this.nc_p1, "number");
AssertType(this, "this");
AssertType(this.nc_p2(value), "number");
AssertType(this.nc_p2, "(number) => number");
AssertType(this, "this");
AssertType(value, "number");
    }
    private nc_pp1: number;
    private nc_pp2(b: number) {
AssertType(this.nc_pp1 + b, "number");
AssertType(this.nc_pp1, "number");
AssertType(this, "this");
AssertType(b, "number");
        return this.nc_pp1 + b;
    }
    private get nc_pp3() {
AssertType(this.nc_pp2(this.nc_pp1), "number");
AssertType(this.nc_pp2, "(number) => number");
AssertType(this, "this");
AssertType(this.nc_pp1, "number");
AssertType(this, "this");
        return this.nc_pp2(this.nc_pp1);
    }
    private set nc_pp3(value: number) {
        this.nc_pp1 = this.nc_pp2(value);
AssertType(this.nc_pp1 = this.nc_pp2(value), "number");
AssertType(this.nc_pp1, "number");
AssertType(this, "this");
AssertType(this.nc_pp2(value), "number");
AssertType(this.nc_pp2, "(number) => number");
AssertType(this, "this");
AssertType(value, "number");
    }
    static nc_s1: number;
    static nc_s2(b: number) {
AssertType(c1.nc_s1 + b, "number");
AssertType(c1.nc_s1, "number");
AssertType(b, "number");
        return c1.nc_s1 + b;
    }
    static get nc_s3() {
AssertType(c1.nc_s2(c1.nc_s1), "number");
AssertType(c1.nc_s2, "(number) => number");
AssertType(c1.nc_s1, "number");
        return c1.nc_s2(c1.nc_s1);
    }
    static set nc_s3(value: number) {
        c1.nc_s1 = c1.nc_s2(value);
AssertType(c1.nc_s1 = c1.nc_s2(value), "number");
AssertType(c1.nc_s1, "number");
AssertType(c1.nc_s2(value), "number");
AssertType(c1.nc_s2, "(number) => number");
AssertType(value, "number");
    }

    // p1 is property of c1
    public a_p1: number;
    // sum with property
    public a_p2(b: number) {
AssertType(this.a_p1 + b, "number");
AssertType(this.a_p1, "number");
AssertType(this, "this");
AssertType(b, "number");
        return this.a_p1 + b;
    }
    // getter property
    public get a_p3() {
AssertType(this.a_p2(this.a_p1), "number");
AssertType(this.a_p2, "(number) => number");
AssertType(this, "this");
AssertType(this.a_p1, "number");
AssertType(this, "this");
        return this.a_p2(this.a_p1);
    }
    // setter property
    public set a_p3(value: number) {
        this.a_p1 = this.a_p2(value);
AssertType(this.a_p1 = this.a_p2(value), "number");
AssertType(this.a_p1, "number");
AssertType(this, "this");
AssertType(this.a_p2(value), "number");
AssertType(this.a_p2, "(number) => number");
AssertType(this, "this");
AssertType(value, "number");
    }
    // pp1 is property of c1
    private a_pp1: number;
    // sum with property
    private a_pp2(b: number) {
AssertType(this.a_p1 + b, "number");
AssertType(this.a_p1, "number");
AssertType(this, "this");
AssertType(b, "number");
        return this.a_p1 + b;
    }
    // getter property
    private get a_pp3() {
AssertType(this.a_pp2(this.a_pp1), "number");
AssertType(this.a_pp2, "(number) => number");
AssertType(this, "this");
AssertType(this.a_pp1, "number");
AssertType(this, "this");
        return this.a_pp2(this.a_pp1);
    }
    // setter property
    private set a_pp3(value: number) {
        this.a_pp1 = this.a_pp2(value);
AssertType(this.a_pp1 = this.a_pp2(value), "number");
AssertType(this.a_pp1, "number");
AssertType(this, "this");
AssertType(this.a_pp2(value), "number");
AssertType(this.a_pp2, "(number) => number");
AssertType(this, "this");
AssertType(value, "number");
    }
    
    // s1 is static property of c1
    static a_s1: number;
    // static sum with property
    static a_s2(b: number) {
AssertType(c1.a_s1 + b, "number");
AssertType(c1.a_s1, "number");
AssertType(b, "number");
        return c1.a_s1 + b;
    }
    // static getter property
    static get a_s3() {
AssertType(c1.s2(c1.s1), "number");
AssertType(c1.s2, "(number) => number");
AssertType(c1.s1, "number");
        return c1.s2(c1.s1);
    }
    
    // setter property
    static set a_s3(value: number) {
        c1.a_s1 = c1.a_s2(value);
AssertType(c1.a_s1 = c1.a_s2(value), "number");
AssertType(c1.a_s1, "number");
AssertType(c1.a_s2(value), "number");
AssertType(c1.a_s2, "(number) => number");
AssertType(value, "number");
    }

    /** p1 is property of c1 */
    public b_p1: number;
    /** sum with property */
    public b_p2(b: number) {
AssertType(this.b_p1 + b, "number");
AssertType(this.b_p1, "number");
AssertType(this, "this");
AssertType(b, "number");
        return this.b_p1 + b;
    }
    /** getter property */
    public get b_p3() {
AssertType(this.b_p2(this.b_p1), "number");
AssertType(this.b_p2, "(number) => number");
AssertType(this, "this");
AssertType(this.b_p1, "number");
AssertType(this, "this");
        return this.b_p2(this.b_p1);
    }
    /** setter property */
    public set b_p3(value: number) {
        this.b_p1 = this.b_p2(value);
AssertType(this.b_p1 = this.b_p2(value), "number");
AssertType(this.b_p1, "number");
AssertType(this, "this");
AssertType(this.b_p2(value), "number");
AssertType(this.b_p2, "(number) => number");
AssertType(this, "this");
AssertType(value, "number");
    }
    /** pp1 is property of c1 */
    private b_pp1: number;
    /** sum with property */
    private b_pp2(b: number) {
AssertType(this.b_p1 + b, "number");
AssertType(this.b_p1, "number");
AssertType(this, "this");
AssertType(b, "number");
        return this.b_p1 + b;
    }
    /** getter property */
    private get b_pp3() {
AssertType(this.b_pp2(this.b_pp1), "number");
AssertType(this.b_pp2, "(number) => number");
AssertType(this, "this");
AssertType(this.b_pp1, "number");
AssertType(this, "this");
        return this.b_pp2(this.b_pp1);
    }
    /** setter property */
    private set b_pp3(value: number) {
        this.b_pp1 = this.b_pp2(value);
AssertType(this.b_pp1 = this.b_pp2(value), "number");
AssertType(this.b_pp1, "number");
AssertType(this, "this");
AssertType(this.b_pp2(value), "number");
AssertType(this.b_pp2, "(number) => number");
AssertType(this, "this");
AssertType(value, "number");
    }
    
    /** s1 is static property of c1 */
    static b_s1: number;
    /** static sum with property */
    static b_s2(b: number) {
AssertType(c1.b_s1 + b, "number");
AssertType(c1.b_s1, "number");
AssertType(b, "number");
        return c1.b_s1 + b;
    }
    /** static getter property 
    */
    static get b_s3() {
AssertType(c1.s2(c1.s1), "number");
AssertType(c1.s2, "(number) => number");
AssertType(c1.s1, "number");
        return c1.s2(c1.s1);
    }
    
    /** setter property 
    */
    static set b_s3(value: number) {
        /** setter */
        c1.b_s1 = c1.b_s2(value);
AssertType(c1.b_s1 = c1.b_s2(value), "number");
AssertType(c1.b_s1, "number");
AssertType(c1.b_s2(value), "number");
AssertType(c1.b_s2, "(number) => number");
AssertType(value, "number");
    }
}
let i1 = new c1();
AssertType(i1, "c1");
AssertType(new c1(), "c1");
AssertType(c1, "typeof c1");

let i1_p = i1.p1;
AssertType(i1_p, "number");
AssertType(i1.p1, "number");

let i1_f = i1.p2;
AssertType(i1_f, "(number) => number");
AssertType(i1.p2, "(number) => number");

let i1_r = i1.p2(20);
AssertType(i1_r, "number");
AssertType(i1.p2(20), "number");
AssertType(i1.p2, "(number) => number");
AssertType(20, "int");

let i1_prop = i1.p3;
AssertType(i1_prop, "number");
AssertType(i1.p3, "number");

i1.p3 = i1_prop;
AssertType(i1.p3 = i1_prop, "number");
AssertType(i1.p3, "number");
AssertType(i1_prop, "number");

let i1_nc_p = i1.nc_p1;
AssertType(i1_nc_p, "number");
AssertType(i1.nc_p1, "number");

let i1_ncf = i1.nc_p2;
AssertType(i1_ncf, "(number) => number");
AssertType(i1.nc_p2, "(number) => number");

let i1_ncr = i1.nc_p2(20);
AssertType(i1_ncr, "number");
AssertType(i1.nc_p2(20), "number");
AssertType(i1.nc_p2, "(number) => number");
AssertType(20, "int");

let i1_ncprop = i1.nc_p3;
AssertType(i1_ncprop, "number");
AssertType(i1.nc_p3, "number");

i1.nc_p3 = i1_ncprop;
AssertType(i1.nc_p3 = i1_ncprop, "number");
AssertType(i1.nc_p3, "number");
AssertType(i1_ncprop, "number");

let i1_s_p = c1.s1;
AssertType(i1_s_p, "number");
AssertType(c1.s1, "number");

let i1_s_f = c1.s2;
AssertType(i1_s_f, "(number) => number");
AssertType(c1.s2, "(number) => number");

let i1_s_r = c1.s2(20);
AssertType(i1_s_r, "number");
AssertType(c1.s2(20), "number");
AssertType(c1.s2, "(number) => number");
AssertType(20, "int");

let i1_s_prop = c1.s3;
AssertType(i1_s_prop, "number");
AssertType(c1.s3, "number");

c1.s3 = i1_s_prop;
AssertType(c1.s3 = i1_s_prop, "number");
AssertType(c1.s3, "number");
AssertType(i1_s_prop, "number");

let i1_s_nc_p = c1.nc_s1;
AssertType(i1_s_nc_p, "number");
AssertType(c1.nc_s1, "number");

let i1_s_ncf = c1.nc_s2;
AssertType(i1_s_ncf, "(number) => number");
AssertType(c1.nc_s2, "(number) => number");

let i1_s_ncr = c1.nc_s2(20);
AssertType(i1_s_ncr, "number");
AssertType(c1.nc_s2(20), "number");
AssertType(c1.nc_s2, "(number) => number");
AssertType(20, "int");

let i1_s_ncprop = c1.nc_s3;
AssertType(i1_s_ncprop, "number");
AssertType(c1.nc_s3, "number");

c1.nc_s3 = i1_s_ncprop;
AssertType(c1.nc_s3 = i1_s_ncprop, "number");
AssertType(c1.nc_s3, "number");
AssertType(i1_s_ncprop, "number");

let i1_c = c1;
AssertType(i1_c, "typeof c1");
AssertType(c1, "typeof c1");

class cProperties {
    private val: number;
    /** getter only property*/
    public get p1() {
AssertType(this.val, "number");
AssertType(this, "this");
        return this.val;

    } // trailing comment of only getter
    public get nc_p1() {
AssertType(this.val, "number");
AssertType(this, "this");
        return this.val;
    }
    /**setter only property*/
    public set p2(value: number) {
        this.val = value;
AssertType(this.val = value, "number");
AssertType(this.val, "number");
AssertType(this, "this");
AssertType(value, "number");
    }
    public set nc_p2(value: number) {
        this.val = value;
AssertType(this.val = value, "number");
AssertType(this.val, "number");
AssertType(this, "this");
AssertType(value, "number");

    } /* trailing comment of setter only*/

    public x = 10; /*trailing comment for property*/
    private y = 10; // trailing comment of // style
}
let cProperties_i = new cProperties();
AssertType(cProperties_i, "cProperties");
AssertType(new cProperties(), "cProperties");
AssertType(cProperties, "typeof cProperties");

cProperties_i.p2 = cProperties_i.p1;
AssertType(cProperties_i.p2 = cProperties_i.p1, "number");
AssertType(cProperties_i.p2, "number");
AssertType(cProperties_i.p1, "number");

cProperties_i.nc_p2 = cProperties_i.nc_p1;
AssertType(cProperties_i.nc_p2 = cProperties_i.nc_p1, "number");
AssertType(cProperties_i.nc_p2, "number");
AssertType(cProperties_i.nc_p1, "number");


