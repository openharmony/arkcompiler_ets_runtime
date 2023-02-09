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

// === tests/cases/compiler/_apply.js ===
declare function AssertType(value:any, type:string):void;
/**
 * A faster alternative to `Function#apply`, this function invokes `func`
 * with the `this` binding of `thisArg` and the arguments of `args`.
 *
 * @private
 * @param {Function} func The function to invoke.
 * @param {*} thisArg The `this` binding of `func`.
 * @param {...*} args The arguments to invoke `func` with.
 * @
returns {*} Returns the result of `func`.
 */
function apply(func, thisArg, ...args) {
    let length = args.length;
AssertType(length, "number");
AssertType(args.length, "number");

    switch (length) {
AssertType(length, "number");

        case 0: 
AssertType(0, "int");
AssertType(func.call(thisArg), "any");
AssertType(func.call, "(Function, any, ...any[]) => any");
AssertType(thisArg, "any");
return func.call(thisArg);

        case 1: 
AssertType(1, "int");
AssertType(func.call(thisArg, args[0]), "any");
AssertType(func.call, "(Function, any, ...any[]) => any");
AssertType(thisArg, "any");
AssertType(args[0], "any");
AssertType(args, "any[]");
AssertType(0, "int");
return func.call(thisArg, args[0]);

        case 2: 
AssertType(2, "int");
AssertType(func.call(thisArg, args[0], args[1]), "any");
AssertType(func.call, "(Function, any, ...any[]) => any");
AssertType(thisArg, "any");
AssertType(args[0], "any");
AssertType(args, "any[]");
AssertType(0, "int");
AssertType(args[1], "any");
AssertType(args, "any[]");
AssertType(1, "int");
return func.call(thisArg, args[0], args[1]);

        case 3: 
AssertType(3, "int");
AssertType(func.call(thisArg, args[0], args[1], args[2]), "any");
AssertType(func.call, "(Function, any, ...any[]) => any");
AssertType(thisArg, "any");
AssertType(args[0], "any");
AssertType(args, "any[]");
AssertType(0, "int");
AssertType(args[1], "any");
AssertType(args, "any[]");
AssertType(1, "int");
AssertType(args[2], "any");
AssertType(args, "any[]");
AssertType(2, "int");
return func.call(thisArg, args[0], args[1], args[2]);
    }
AssertType(func.apply(thisArg, args), "any");
AssertType(func.apply, "(Function, any, ?any) => any");
AssertType(thisArg, "any");
AssertType(args, "any[]");
    return func.apply(thisArg, args);
}

export default apply;

