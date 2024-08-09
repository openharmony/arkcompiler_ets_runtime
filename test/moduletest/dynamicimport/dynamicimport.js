/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
// Ordinary dynamic import test
const objA = {
    toString() {
        return './A.js';
    }
};
const objB = {
    toString() {
        return './B.js';
    }
};

import(objA).then(animalA => {
    print('full name : ', animalA.name);
    print(animalA.familyCrets);
}).then().catch();

async function f() {
    var animalB = await import(objB);
    print('full name : ', animalB.name);
    print(animalB.familyCrets);
}
f().then().catch();

// Import script file test
import("./C.js").then(m => {
    print(JSON.stringify(m));
}).then();

// Catch error test
import('./throwerror.js').catch(error => {
    print(error.name);
}).then();

var _v_1 = {};
function _f_8() {
    try{
        _f_8();
    } catch (e) {
        import(_v_1).then();
    }
}
_f_8();

import('./1.json').then(m => {
    print(m.default.name);
}).then().catch();

import('@native:system.app').then(m => {
    print("native module");
}).then().catch();

import('@native:system.app').then(m => {
    print("native module");
}).then().catch();
// Dynamic Loading Reload Test
import('./aaa.js').then(m => {
    print("dynamic import aaa.js 1st time");
})
import('./aaa.js').then(m => {
    print("dynamic import aaa.js 2nd time");
})
