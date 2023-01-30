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

// === tests/cases/compiler/objectLitGetterSetter.ts ===
declare function AssertType(value:any, type:string):void;
            let obj = {};
AssertType(obj, "{}");
AssertType({}, "{}");

            Object.defineProperty(obj, "accProperty", <PropertyDescriptor>({
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(obj, "{}");
AssertType("accProperty", "string");
AssertType(<PropertyDescriptor>({                get: function () {                    eval("public = 1;");                    return 11;                },                set: function (v) {                }            }), "PropertyDescriptor");
AssertType(({                get: function () {                    eval("public = 1;");                    return 11;                },                set: function (v) {                }            }), "{ get: () => number; set: (any) => void; }");
AssertType({                get: function () {                    eval("public = 1;");                    return 11;                },                set: function (v) {                }            }, "{ get: () => number; set: (any) => void; }");
AssertType(Object.defineProperty(obj, "accProperty", <PropertyDescriptor>({                get: function () {                    eval("public = 1;");                    return 11;                },                set: function (v) {                }            })), "{}");

                get: function () {
AssertType(get, "() => number");
AssertType(function () {                    eval("public = 1;");                    return 11;                }, "() => number");

                    eval("public = 1;");
AssertType(eval("public = 1;"), "any");
AssertType(eval, "(string) => any");
AssertType("public = 1;", "string");

AssertType(11, "int");
                    return 11;

                },
                set: function (v) {
AssertType(set, "(any) => void");
AssertType(function (v) {                }, "(any) => void");
AssertType(v, "any");
                }
            }))


