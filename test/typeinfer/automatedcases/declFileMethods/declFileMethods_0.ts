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

// === tests/cases/compiler/declFileMethods_0.ts ===
declare function AssertType(value:any, type:string):void;
export class c1 {
    /** This comment should appear for foo*/
    public foo() {
    }
    /** This is comment for function signature*/
    public fooWithParameters(/** this is comment about a*/a: string,
        /** this is comment for b*/
        b: number) {
        let d = a;
AssertType(d, "string");
AssertType(a, "string");
    }
    public fooWithRestParameters(a: string, ...rests: string[]) {
AssertType(a + rests.join(""), "string");
AssertType(a, "string");
AssertType(rests.join(""), "string");
AssertType(rests.join, "(?string) => string");
AssertType("", "string");
        return a + rests.join("");
    }

    public fooWithOverloads(a: string): string;
    public fooWithOverloads(a: number): number;
    public fooWithOverloads(a: any): any {
AssertType(a, "any");
        return a;
    }


    /** This comment should appear for privateFoo*/
    private privateFoo() {
    }
    /** This is comment for function signature*/
    private privateFooWithParameters(/** this is comment about a*/a: string,
        /** this is comment for b*/
        b: number) {
        let d = a;
AssertType(d, "string");
AssertType(a, "string");
    }
    private privateFooWithRestParameters(a: string, ...rests: string[]) {
AssertType(a + rests.join(""), "string");
AssertType(a, "string");
AssertType(rests.join(""), "string");
AssertType(rests.join, "(?string) => string");
AssertType("", "string");
        return a + rests.join("");
    }
    private privateFooWithOverloads(a: string): string;
    private privateFooWithOverloads(a: number): number;
    private privateFooWithOverloads(a: any): any {
AssertType(a, "any");
        return a;
    }


    /** This comment should appear for static foo*/
    static staticFoo() {
    }
    /** This is comment for function signature*/
    static staticFooWithParameters(/** this is comment about a*/a: string,
        /** this is comment for b*/
        b: number) {
        let d = a;
AssertType(d, "string");
AssertType(a, "string");
    }
    static staticFooWithRestParameters(a: string, ...rests: string[]) {
AssertType(a + rests.join(""), "string");
AssertType(a, "string");
AssertType(rests.join(""), "string");
AssertType(rests.join, "(?string) => string");
AssertType("", "string");
        return a + rests.join("");
    }
    static staticFooWithOverloads(a: string): string;
    static staticFooWithOverloads(a: number): number;
    static staticFooWithOverloads(a: any): any {
AssertType(a, "any");
        return a;
    }


    /** This comment should appear for privateStaticFoo*/
    private static privateStaticFoo() {
    }
    /** This is comment for function signature*/
    private static privateStaticFooWithParameters(/** this is comment about a*/a: string,
        /** this is comment for b*/
        b: number) {
        let d = a;
AssertType(d, "string");
AssertType(a, "string");
    }
    private static privateStaticFooWithRestParameters(a: string, ...rests: string[]) {
AssertType(a + rests.join(""), "string");
AssertType(a, "string");
AssertType(rests.join(""), "string");
AssertType(rests.join, "(?string) => string");
AssertType("", "string");
        return a + rests.join("");
    }
    private static privateStaticFooWithOverloads(a: string): string;
    private static privateStaticFooWithOverloads(a: number): number;
    private static privateStaticFooWithOverloads(a: any): any {
AssertType(a, "any");
        return a;
    }
}

export interface I1 {
    /** This comment should appear for foo*/
    foo(): string;

    /** This is comment for function signature*/
    fooWithParameters(/** this is comment about a*/a: string,
        /** this is comment for b*/
        b: number): void;

    fooWithRestParameters(a: string, ...rests: string[]): string;

    fooWithOverloads(a: string): string;
    fooWithOverloads(a: number): number;
}


