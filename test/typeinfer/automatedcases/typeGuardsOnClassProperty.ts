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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsOnClassProperty.ts ===
declare function AssertType(value:any, type:string):void;
// Note that type guards affect types of letiables and parameters only and 
// have no effect on members of objects such as properties. 

// Note that the class's property must be copied to a local letiable for
// the type guard to have an effect
class D {
    data: string | string[];
    getData() {
        let data = this.data;
AssertType(data, "union");
AssertType(this.data, "union");
AssertType(this, "this");

AssertType(typeof data === "string" ? data : data.join(" "), "string");
AssertType(typeof data === "string", "boolean");
AssertType(typeof data, "union");
AssertType(data, "union");
AssertType("string", "string");
AssertType(data, "string");
AssertType(data.join(" "), "string");
AssertType(data.join, "(?string) => string");
AssertType(" ", "string");
        return typeof data === "string" ? data : data.join(" ");
    }

    getData1() {
AssertType(typeof this.data === "string" ? this.data : this.data.join(" "), "string");
AssertType(typeof this.data === "string", "boolean");
AssertType(typeof this.data, "union");
AssertType(this.data, "union");
AssertType(this, "this");
AssertType("string", "string");
AssertType(this.data, "string");
AssertType(this, "this");
AssertType(this.data.join(" "), "string");
AssertType(this.data.join, "(?string) => string");
AssertType(this.data, "string[]");
AssertType(this, "this");
AssertType(" ", "string");
        return typeof this.data === "string" ? this.data : this.data.join(" ");
    }
}

let o: {
AssertType(o, "{ prop1: union; prop2: union; }");

    prop1: number|string;
AssertType(prop1, "union");

    prop2: boolean|string;
AssertType(prop2, "union");

} = {
AssertType({        prop1: "string" ,        prop2: true    }, "{ prop1: string; prop2: true; }");

        prop1: "string" ,
AssertType(prop1, "string");
AssertType("string", "string");

        prop2: true
AssertType(prop2, "boolean");
AssertType(true, "boolean");
    }

if (typeof o.prop1 === "string" && o.prop1.toLowerCase()) {}
let prop1 = o.prop1;
AssertType(prop1, "union");
AssertType(o.prop1, "union");

if (typeof prop1 === "string" && prop1.toLocaleLowerCase()) { }

