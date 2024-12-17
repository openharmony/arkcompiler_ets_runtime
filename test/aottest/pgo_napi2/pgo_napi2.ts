/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 */

// test Change Object.prototype
function test7() {
    let obj = ArkTools.createNapiObject();
    let obj2 = {};
    Object.prototype.x = 1;
    print(obj.x);
    obj2.__proto__.y = 2;
    print(obj.y);
}
test7();
print(ArkTools.isAOTCompiled(test7))
print(ArkTools.isAOTDeoptimized(test7))

function test8() {
    var Class = {};
    Class = function () {
        this.listeners = {};
    };
    Class.prototype = {
        add: function () {
        },
    };
    var EventBus = new Class();
}
test8();

// test change __proto__ after transition
function test9() {
    let obj = ArkTools.createNapiObject();
    obj.x = 1;
    obj.__proto__ = {};
    print(obj.x);
}
test9();
print(ArkTools.isAOTCompiled(test9))
print(ArkTools.isAOTDeoptimized(test9))

// test transition's __proto__
function test10() {
    let obj = ArkTools.createNapiObject();
    obj.x = 1;
    print(obj.__proto__.toString());
}
test10();
print(ArkTools.isAOTCompiled(test10))
print(ArkTools.isAOTDeoptimized(test10))
  