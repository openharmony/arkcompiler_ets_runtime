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

// === tests/cases/conformance/salsa/a.js ===
declare function AssertType(value:any, type:string):void;
function Multimap4() {
  this._map = {};
AssertType(this._map = {}, "{}");
AssertType(this._map, "any");
AssertType(this, "this");
AssertType({}, "{}");

};

Multimap4["prototype"] = {
AssertType(Multimap4["prototype"], "{ get(string): number; }");
AssertType(Multimap4, "typeof Multimap4");
AssertType("prototype", "string");
AssertType({  /**   * @param {string} key   * @returns {number} the value ok   */  get(key) {    return this._map[key + ''];  }}, "{ get(string): number; }");
AssertType(Multimap4["prototype"] = {  /**   * @param {string} key   * @returns {number} the value ok   */  get(key) {    return this._map[key + ''];  }}, "{ get(string): number; }");

  /**
   * @param {string} key
   * @
returns {number} the value ok
   */
  get(key) {
AssertType(get, "(string) => number");
AssertType(key, "string");

AssertType(this._map[key + ''], "any");
AssertType(this._map, "{}");
AssertType(this, "this");
AssertType(key + '', "string");
AssertType(key, "string");
AssertType('', "string");
    return this._map[key + ''];
  }
};

Multimap4["prototype"]["add-on"] = function() {};
AssertType(Multimap4["prototype"]["add-on"] = function() {}, "() => void");
AssertType(Multimap4["prototype"]["add-on"], "any");
AssertType(Multimap4["prototype"], "{ get(string): number; }");
AssertType(Multimap4, "typeof Multimap4");
AssertType("prototype", "string");
AssertType("add-on", "string");
AssertType(function() {}, "() => void");

Multimap4["prototype"]["addon"] = function() {};
AssertType(Multimap4["prototype"]["addon"] = function() {}, "() => void");
AssertType(Multimap4["prototype"]["addon"], "any");
AssertType(Multimap4["prototype"], "{ get(string): number; }");
AssertType(Multimap4, "typeof Multimap4");
AssertType("prototype", "string");
AssertType("addon", "string");
AssertType(function() {}, "() => void");

Multimap4["prototype"]["__underscores__"] = function() {};
AssertType(Multimap4["prototype"]["__underscores__"] = function() {}, "() => void");
AssertType(Multimap4["prototype"]["__underscores__"], "any");
AssertType(Multimap4["prototype"], "{ get(string): number; }");
AssertType(Multimap4, "typeof Multimap4");
AssertType("prototype", "string");
AssertType("__underscores__", "string");
AssertType(function() {}, "() => void");

const map4 = new Multimap4();
AssertType(map4, "Multimap4");
AssertType(new Multimap4(), "Multimap4");
AssertType(Multimap4, "typeof Multimap4");

map4.get("");
AssertType(map4.get(""), "number");
AssertType(map4.get, "(string) => number");
AssertType("", "string");

map4["add-on"]();
AssertType(map4["add-on"](), "void");
AssertType(map4["add-on"], "() => void");
AssertType(map4, "Multimap4");
AssertType("add-on", "string");

map4.addon();
AssertType(map4.addon(), "void");
AssertType(map4.addon, "() => void");

map4.__underscores__();
AssertType(map4.__underscores__(), "void");
AssertType(map4.__underscores__, "() => void");


