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

// === tests/cases/compiler/library_ObjectPrototypeProperties.ts ===
declare function AssertType(value:any, type:string):void;
// Properties of the Object Prototype Object as per ES5 spec
// http://www.ecma-international.org/ecma-262/5.1/#sec-15.2.4
Object.prototype.constructor;
AssertType(Object.prototype.constructor, "Function");

Object.prototype.toString();
AssertType(Object.prototype.toString(), "string");
AssertType(Object.prototype.toString, "() => string");

Object.prototype.toLocaleString();
AssertType(Object.prototype.toLocaleString(), "string");
AssertType(Object.prototype.toLocaleString, "() => string");

Object.prototype.valueOf();
AssertType(Object.prototype.valueOf(), "Object");
AssertType(Object.prototype.valueOf, "() => Object");

Object.prototype.hasOwnProperty("string");
AssertType(Object.prototype.hasOwnProperty("string"), "boolean");
AssertType(Object.prototype.hasOwnProperty, "(PropertyKey) => boolean");
AssertType("string", "string");

Object.prototype.isPrototypeOf(Object);
AssertType(Object.prototype.isPrototypeOf(Object), "boolean");
AssertType(Object.prototype.isPrototypeOf, "(Object) => boolean");
AssertType(Object, "ObjectConstructor");

Object.prototype.propertyIsEnumerable("string");
AssertType(Object.prototype.propertyIsEnumerable("string"), "boolean");
AssertType(Object.prototype.propertyIsEnumerable, "(PropertyKey) => boolean");
AssertType("string", "string");


