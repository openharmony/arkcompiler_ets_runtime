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

// load load
function test1() {
    let obj = ArkTools.createNapiObject();
    print(obj.x);   // load: slow
    print(obj.y);   // load: slow
}
test1();
print(ArkTools.isAOTCompiled(test1))
print(ArkTools.isAOTDeoptimized(test1))

// load store load store load load
function test2() {
    let obj = ArkTools.createNapiObject();
    print(obj.x);   // load: slow
    obj.x = 1;  // store: fast
    print(obj.x);   // load: fast
    obj.y = 2;  // store: fast
    print(obj.y);   // load: fast
    print(obj.z);   // load: slow
}
test2();
print(ArkTools.isAOTCompiled(test2))
print(ArkTools.isAOTDeoptimized(test2))

// store store load load load
function test3() {
    let obj = ArkTools.createNapiObject();
    obj.x = 1;  // store: fast
    obj.y = 1;  // store: fast
    print(obj.x);   // load: fast
    print(obj.y);   // load: fast
    print(obj.z);   // load: slow
}
test3();
print(ArkTools.isAOTCompiled(test3))
print(ArkTools.isAOTDeoptimized(test3))

// test __proto__
function test4() {
    let objproto = ArkTools.createNapiObject();
    let obj = ArkTools.createNapiObject();
    obj.__proto__ = objproto;   // store: slow
    objproto.x = 1;    // store: slow
    objproto.y = 1;    // store: slow
}
test4();
print(ArkTools.isAOTCompiled(test4))
print(ArkTools.isAOTDeoptimized(test4))

ArkTools.printTypedOpProfiler("MONO_STORE_PROPERTY");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.clearTypedOpProfiler();

// test for
function test5() {
    let obj = ArkTools.createNapiObject();
    obj.x = 1;  // store: fast
    for (let i = 0; i < 100; i++) {
        obj.x;          // load: fast
    }
    ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
    ArkTools.clearTypedOpProfiler();
}
test5();
print(ArkTools.isAOTCompiled(test5))
print(ArkTools.isAOTDeoptimized(test5))

// test normal js object
function test6() {
    let obj = ArkTools.createNapiObject();
    let obj2 = {};
    obj.x = 1;  // store: fast
    obj2.x = 1;     // store: slow
    print(obj.x);   // load: fast
    print(obj.y);   // load: slow
    print(obj2.x);  // load: slow
    print(obj2.y);  // load: slow
    ArkTools.printTypedOpProfiler("MONO_STORE_PROPERTY");
    ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
    ArkTools.clearTypedOpProfiler();
}
test6();
print(ArkTools.isAOTCompiled(test6))
print(ArkTools.isAOTDeoptimized(test6))
