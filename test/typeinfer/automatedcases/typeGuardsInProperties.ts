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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsInProperties.ts ===
declare function AssertType(value:any, type:string):void;
// Note that type guards affect types of letiables and parameters only and 
// have no effect on members of objects such as properties. 

let num: number;
AssertType(num, "number");

let strOrNum: string | number;
AssertType(strOrNum, "union");

class C1 {
    private pp1: string | number;
    pp2: string | number;
    // Inside public accessor getter
    get pp3() {
AssertType(strOrNum, "union");
        return strOrNum;
    }
    method() {
        strOrNum = typeof this.pp1 === "string" && this.pp1; // string | number
AssertType(strOrNum = typeof this.pp1 === "string" && this.pp1, "string");
AssertType(strOrNum, "union");
AssertType(typeof this.pp1 === "string" && this.pp1, "string");
AssertType(typeof this.pp1 === "string", "boolean");
AssertType(typeof this.pp1, "union");
AssertType(this.pp1, "union");
AssertType(this, "this");
AssertType("string", "string");
AssertType(this.pp1, "string");
AssertType(this, "this");

        strOrNum = typeof this.pp2 === "string" && this.pp2; // string | number
AssertType(strOrNum = typeof this.pp2 === "string" && this.pp2, "string");
AssertType(strOrNum, "union");
AssertType(typeof this.pp2 === "string" && this.pp2, "string");
AssertType(typeof this.pp2 === "string", "boolean");
AssertType(typeof this.pp2, "union");
AssertType(this.pp2, "union");
AssertType(this, "this");
AssertType("string", "string");
AssertType(this.pp2, "string");
AssertType(this, "this");

        strOrNum = typeof this.pp3 === "string" && this.pp3; // string | number
AssertType(strOrNum = typeof this.pp3 === "string" && this.pp3, "string");
AssertType(strOrNum, "union");
AssertType(typeof this.pp3 === "string" && this.pp3, "string");
AssertType(typeof this.pp3 === "string", "boolean");
AssertType(typeof this.pp3, "union");
AssertType(this.pp3, "union");
AssertType(this, "this");
AssertType("string", "string");
AssertType(this.pp3, "string");
AssertType(this, "this");
    }
}
let c1: C1;
AssertType(c1, "C1");

strOrNum = typeof c1.pp2 === "string" && c1.pp2; // string | number
AssertType(strOrNum = typeof c1.pp2 === "string" && c1.pp2, "string");
AssertType(strOrNum, "union");
AssertType(typeof c1.pp2 === "string" && c1.pp2, "string");
AssertType(typeof c1.pp2 === "string", "boolean");
AssertType(typeof c1.pp2, "union");
AssertType(c1.pp2, "union");
AssertType("string", "string");
AssertType(c1.pp2, "string");

strOrNum = typeof c1.pp3 === "string" && c1.pp3; // string | number
AssertType(strOrNum = typeof c1.pp3 === "string" && c1.pp3, "string");
AssertType(strOrNum, "union");
AssertType(typeof c1.pp3 === "string" && c1.pp3, "string");
AssertType(typeof c1.pp3 === "string", "boolean");
AssertType(typeof c1.pp3, "union");
AssertType(c1.pp3, "union");
AssertType("string", "string");
AssertType(c1.pp3, "string");

let obj1: {
AssertType(obj1, "{ x: union; }");

    x: string | number;
AssertType(x, "union");

};
strOrNum = typeof obj1.x === "string" && obj1.x;  // string | number
AssertType(strOrNum = typeof obj1.x === "string" && obj1.x, "string");
AssertType(strOrNum, "union");
AssertType(typeof obj1.x === "string" && obj1.x, "string");
AssertType(typeof obj1.x === "string", "boolean");
AssertType(typeof obj1.x, "union");
AssertType(obj1.x, "union");
AssertType("string", "string");
AssertType(obj1.x, "string");


