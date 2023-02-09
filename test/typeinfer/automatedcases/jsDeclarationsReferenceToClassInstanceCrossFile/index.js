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
const {Rectangle} = require('./rectangle');
AssertType(Rectangle, "typeof Rectangle");
AssertType(require('./rectangle'), "typeof import("tests/cases/conformance/jsdoc/declarations/rectangle")");
AssertType(require, "any");
AssertType('./rectangle', "string");

class Render {
    constructor() {
        /**
         * Object list
         * @type {Rectangle[]}
         */
        this.objects = [];
AssertType(this.objects = [], "undefined[]");
AssertType(this.objects, "Rectangle[]");
AssertType(this, "this");
AssertType([], "undefined[]");
    }
    /**
     * Adds a rectangle
     * 
     * @
returns {Rectangle} the rect
     */
    addRectangle() {
        const obj = new Rectangle();
AssertType(obj, "Rectangle");
AssertType(new Rectangle(), "Rectangle");
AssertType(Rectangle, "typeof Rectangle");

        this.objects.push(obj);
AssertType(this.objects.push(obj), "number");
AssertType(this.objects.push, "(...Rectangle[]) => number");
AssertType(this.objects, "Rectangle[]");
AssertType(this, "this");
AssertType(obj, "Rectangle");

AssertType(obj, "Rectangle");
        return obj;
    }
}

module.exports = { Render };
AssertType(module.exports = { Render }, "typeof module.exports");
AssertType(module.exports, "typeof module.exports");
AssertType({ Render }, "{ Render: typeof Render; }");
AssertType(Render, "typeof Render");


