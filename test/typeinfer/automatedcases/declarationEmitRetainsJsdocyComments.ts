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

// === tests/cases/compiler/declarationEmitRetainsJsdocyComments.ts ===
declare function AssertType(value:any, type:string):void;
/**
 * comment1
 * @param p 
 */
export const foo = (p: string) => {
AssertType(foo, "(string) => { bar: (number) => void; bar2(number): void; }");
AssertType(p, "string");
AssertType((p: string) => {    return {        /**         * comment2         * @param s          */        bar: (s: number) => {},        /**         * comment3         * @param s          */        bar2(s: number) {},    }}, "(string) => { bar: (number) => void; bar2(number): void; }");

AssertType({        /**         * comment2         * @param s          */        bar: (s: number) => {},        /**         * comment3         * @param s          */        bar2(s: number) {},    }, "{ bar: (number) => void; bar2(number): void; }");
    return {

        /**
         * comment2
         * @param s 
         */
        bar: (s: number) => {},
AssertType(bar, "(number) => void");
AssertType((s: number) => {}, "(number) => void");
AssertType(s, "number");

        /**
         * comment3
         * @param s 
         */
        bar2(s: number) {},
AssertType(bar2, "(number) => void");
AssertType(s, "number");
    }
}

export class Foo {
    /**
     * comment4
     * @param s  
     */
    bar(s: number) {
    }
}

export let {
    /**
    * comment5
    */
    someMethod
AssertType(someMethod, "any");

} = null as any;
AssertType(null as any, "any");
AssertType(null, "null");

declare global {
    interface ExtFunc {
        /**
        * comment6
        */
        someMethod(collection: any[]): boolean;
    }
}


