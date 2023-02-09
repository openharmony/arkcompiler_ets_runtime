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

// === tests/cases/compiler/objectCreate2.ts ===
declare function AssertType(value:any, type:string):void;
declare let union: null | { a: number, b: string };
AssertType(union, "{ a: number; b: string; }");
AssertType(null, "null");
AssertType(a, "number");
AssertType(b, "string");

let n = Object.create(null);             // any
AssertType(n, "any");
AssertType(Object.create(null), "any");
AssertType(Object.create, "{ (object): any; (object, PropertyDescriptorMap & ThisType<any>): any; }");
AssertType(null, "null");

let t = Object.create({ a: 1, b: "" });  // {a: number, b: string 
AssertType(t, "any");

AssertType(Object.create({ a: 1, b: "" }), "any");

AssertType(Object.create, "{ (object): any; (object, PropertyDescriptorMap & ThisType<any>): any; }");

AssertType({ a: 1, b: "" }, "{ a: number; b: string; }");

AssertType(a, "number");

AssertType(1, "int");

AssertType(b, "string");

AssertType("", "string");
}

let u = Object.create(union);            // {a: number, b: string 
AssertType(u, "any");

AssertType(Object.create(union), "any");

AssertType(Object.create, "{ (object): any; (object, PropertyDescriptorMap & ThisType<any>): any; }");

AssertType(union, "{ a: number; b: string; }");
}

let e = Object.create({});               // {
AssertType(e, "any");

AssertType(Object.create({}), "any");

AssertType(Object.create, "{ (object): any; (object, PropertyDescriptorMap & ThisType<any>): any; }");

AssertType({}, "{}");
}

let o = Object.create(<object>{});       // object
AssertType(o, "any");
AssertType(Object.create(<object>{}), "any");
AssertType(Object.create, "{ (object): any; (object, PropertyDescriptorMap & ThisType<any>): any; }");
AssertType(<object>{}, "object");
AssertType({}, "{}");

let a = Object.create(null, {});           // any
AssertType(a, "any");
AssertType(Object.create(null, {}), "any");
AssertType(Object.create, "{ (object): any; (object, PropertyDescriptorMap & ThisType<any>): any; }");
AssertType(null, "null");
AssertType({}, "{}");

let a = Object.create({ a: 1, b: "" }, {});
AssertType(a, "any");
AssertType(Object.create({ a: 1, b: "" }, {}), "any");
AssertType(Object.create, "{ (object): any; (object, PropertyDescriptorMap & ThisType<any>): any; }");
AssertType({ a: 1, b: "" }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "string");
AssertType("", "string");
AssertType({}, "{}");

let a = Object.create(union, {});
AssertType(a, "any");
AssertType(Object.create(union, {}), "any");
AssertType(Object.create, "{ (object): any; (object, PropertyDescriptorMap & ThisType<any>): any; }");
AssertType(union, "{ a: number; b: string; }");
AssertType({}, "{}");

let a = Object.create({}, {});
AssertType(a, "any");
AssertType(Object.create({}, {}), "any");
AssertType(Object.create, "{ (object): any; (object, PropertyDescriptorMap & ThisType<any>): any; }");
AssertType({}, "{}");
AssertType({}, "{}");

let a = Object.create(<object>{}, {});
AssertType(a, "any");
AssertType(Object.create(<object>{}, {}), "any");
AssertType(Object.create, "{ (object): any; (object, PropertyDescriptorMap & ThisType<any>): any; }");
AssertType(<object>{}, "object");
AssertType({}, "{}");
AssertType({}, "{}");


