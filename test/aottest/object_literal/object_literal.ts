/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

// test objectLiteral transition
function test() {
    let obj = {a: 1, b: 2};
    obj.c = 3;
    print(obj.c);
}
test();
print(ArkTools.isAOTCompiled(test))
print(ArkTools.isAOTDeoptimized(test))

// test whether the ObjectLiteralHClassCache is right
function test2() {
    let obj0 = ArkTools.createNapiObject();
    let d = {x: 3, y: 4};
    function foo() {
        obj0.x = 1;
        print(obj0.x);
    }
    foo();
    if (ArkTools.isAOTCompiled(foo)) {
        d = {x: 1, y: 2};
    }
    print(d.x);
    print(d.y);
}
test2();
print(ArkTools.isAOTCompiled(test2))
print(ArkTools.isAOTDeoptimized(test2))

// test object literal with same literal length
function test3() {
    let obj = {name: 1};
    function foo(obj) {
        print(obj.name);
    }
    foo(obj);
    if (ArkTools.isAOTCompiled(test)) {
        let obj2 = {name: 2};
        foo(obj2);
    }
    function test() {
        obj.a = 1;
        obj.b = 2;
        obj.c = 3;
        obj.d = 4;
    }
    test();
}
test3();
print(ArkTools.isAOTCompiled(test3))
print(ArkTools.isAOTDeoptimized(test3))

// test object literal has more or less properties
function test4() {
    let obj = {name: 1};
    obj.x = 1;
    obj.y = 2;
    obj.z = 3;
    // an object literal has more properties.
    let obj1 = {key: 1};
    obj1.x = 1;
    obj1.y = 2;
    obj1.z = 3;
    obj1.a = 4;
    // an object literal has less properties.
    let obj2 = {value: 1};
    obj2.x = 1;
    print(obj.name);
    print(obj1.a);
    print(obj2.x);
}
test4();
print(ArkTools.isAOTCompiled(test4))
print(ArkTools.isAOTDeoptimized(test4))

// test AOT in-prop is hole
function test5() {
    function Base() {}
    Base.prototype = {
        // constructor is hole
        set constructor(_) {
        }
    };
    new Base();
}
test5();
print(ArkTools.isAOTCompiled(test5))

// test AOT CreateObjectWithBuffer
function CreateBigObjectBuffer() {
    let obj = {name1: 1, key1: 2};
    obj.a0 = 0;
    obj.a1 = 1;
    obj.a2 = 2;
    obj.a3 = 3;
    obj.a4 = 4;
    obj.a5 = 5;
    obj.a6 = 6;
    obj.a7 = 7;
    obj.a8 = 8;
    obj.a9 = 0;
    obj.a10 = 10;
    obj.a11 = 11;
}
print(ArkTools.isAOTCompiled(CreateBigObjectBuffer))
print(ArkTools.isAOTDeoptimized(CreateBigObjectBuffer))

function test6() {
    CreateBigObjectBuffer();
    if (ArkTools.isAOTCompiled(CreateBigObjectBuffer)) {
        let obj2 = {name2: 1, key2: 2};
        obj2.key = 2;
    }
}
test6();
print(ArkTools.isAOTCompiled(test6))
print(ArkTools.isAOTDeoptimized(test6))

// test literal length is zero
function test7() {
    var x = {
        0: 0,
        1: 1
    };
    x.a = 2;
    var y = {
        2: 2
    };
    y.a = 1;
    y.b = 2;
    print(x.a);
    print(x.b);
    print(y.a);
    print(y.b);
}
test7();
print(ArkTools.isAOTCompiled(test7))
print(ArkTools.isAOTDeoptimized(test7))
