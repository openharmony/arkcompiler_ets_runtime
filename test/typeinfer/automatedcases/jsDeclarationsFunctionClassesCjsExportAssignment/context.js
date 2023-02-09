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

// === tests/cases/conformance/jsdoc/declarations/context.js ===
declare function AssertType(value:any, type:string):void;
/**
 * Imports
 *
 * @typedef {import("./timer")} Timer
 * @typedef {import("./hook")} Hook
 * @typedef {import("./hook").HookHandler} HookHandler
 */

/**
 * Input type definition
 *
 * @typedef {Object} Input
 * @prop {Timer} timer
 * @prop {Hook} hook
 */
 
/**
 * State type definition
 *
 * @typedef {Object} State
 * @prop {Timer} timer
 * @prop {Hook} hook
 */

/**
 * New `Context`
 *
 * @class
 * @param {Input} input
 */

function Context(input) {
    if (!(this instanceof Context)) {
AssertType(!(this instanceof Context), "boolean");
AssertType((this instanceof Context), "boolean");
AssertType(this instanceof Context, "boolean");
AssertType(this, "this");
AssertType(Context, "typeof Context");

AssertType(new Context(input), "Context");
AssertType(Context, "typeof Context");
AssertType(input, "Input");
      return new Context(input)
    }
    this.state = this.construct(input);
AssertType(this.state = this.construct(input), "State");
AssertType(this.state, "any");
AssertType(this, "this & { construct(Input, ?import("tests/cases/conformance/jsdoc/declarations/hook").HookHandler): State; }");
AssertType(this.construct(input), "State");
AssertType(this.construct, "(Input, ?import("tests/cases/conformance/jsdoc/declarations/hook").HookHandler) => State");
AssertType(this, "this & { construct(Input, ?import("tests/cases/conformance/jsdoc/declarations/hook").HookHandler): State; }");
AssertType(input, "Input");
}
Context.prototype = {
AssertType(Context.prototype, "{ construct(Input, ?union): State; }");
AssertType({    /**     * @param {Input} input     * @param {HookHandler=} handle     * @returns {State}     */    construct(input, handle = () => void 0) {        return input;    }}, "{ construct(Input, ?union): State; }");
AssertType(Context.prototype = {    /**     * @param {Input} input     * @param {HookHandler=} handle     * @returns {State}     */    construct(input, handle = () => void 0) {        return input;    }}, "{ construct(Input, ?union): State; }");

    /**
     * @param {Input} input
     * @param {HookHandler=} handle
     * @
returns {State}
     */
    construct(input, handle = () => void 0) {
AssertType(construct, "(Input, ?union) => State");
AssertType(input, "Input");
AssertType(handle, "import("tests/cases/conformance/jsdoc/declarations/hook").HookHandler");
AssertType(() => void 0, "() => any");
AssertType(void 0, "undefined");
AssertType(0, "int");

AssertType(input, "Input");
        return input;
    }
}
module.exports = Context;
AssertType(module.exports = Context, "{ (Input): Context; new (Input): Context; prototype: { construct(Input, ?import("tests/cases/conformance/jsdoc/declarations/hook").HookHandler): State; }; }");
AssertType(module.exports, "{ (Input): Context; new (Input): Context; prototype: { construct(Input, ?import("tests/cases/conformance/jsdoc/declarations/hook").HookHandler): State; }; }");
AssertType(Context, "typeof Context");


