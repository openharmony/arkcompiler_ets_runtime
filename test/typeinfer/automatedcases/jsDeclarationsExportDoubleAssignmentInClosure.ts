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

// === tests/cases/conformance/jsdoc/declarations/index.js ===
declare function AssertType(value:any, type:string):void;
// @ts-nocheck
function foo() {
    module.exports = exports = function (o) {
AssertType(module.exports, "{ (any): any; methods: () => void; }");
AssertType(exports = function (o) {        return (o == null) ? create(base) : defineProperties(Object(o), descriptors);    }, "(any) => any");
AssertType(exports, "any");
AssertType(function (o) {        return (o == null) ? create(base) : defineProperties(Object(o), descriptors);    }, "(any) => any");
AssertType(o, "any");
AssertType(module.exports = exports = function (o) {        return (o == null) ? create(base) : defineProperties(Object(o), descriptors);    }, "{ (any): any; methods: () => void; }");

AssertType((o == null) ? create(base) : defineProperties(Object(o), descriptors), "any");
AssertType((o == null), "boolean");
AssertType(o == null, "boolean");
AssertType(o, "any");
AssertType(null, "null");
AssertType(create(base), "error");
AssertType(create, "error");
AssertType(base, "error");
AssertType(defineProperties(Object(o), descriptors), "error");
AssertType(defineProperties, "error");
AssertType(Object(o), "any");
AssertType(Object, "ObjectConstructor");
AssertType(o, "any");
AssertType(descriptors, "error");
        return (o == null) ? create(base) : defineProperties(Object(o), descriptors);

    };
    const m = function () {
AssertType(m, "() => void");
AssertType(function () {        // I have no idea what to put here    }, "() => void");

        // I have no idea what to put here
    }
    exports.methods = m;
AssertType(exports.methods = m, "() => void");
AssertType(exports.methods, "any");
AssertType(m, "() => void");
}


