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
const Obj = require("./obj");
AssertType(Obj, "typeof Obj");
AssertType(require("./obj"), "typeof Obj");
AssertType(require, "any");
AssertType("./obj", "string");

class Container {
    constructor() {
        this.usage = new Obj();
AssertType(this.usage = new Obj(), "Obj");
AssertType(this.usage, "any");
AssertType(this, "this");
AssertType(new Obj(), "Obj");
AssertType(Obj, "typeof Obj");
    }
}

module.exports = Container;
AssertType(module.exports = Container, "typeof Container");
AssertType(module.exports, "typeof Container");
AssertType(Container, "typeof Container");


