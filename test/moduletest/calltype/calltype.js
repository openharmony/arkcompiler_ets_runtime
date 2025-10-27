/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

/*
 * @tc.name:calltype
 * @tc.desc:test different kinds of call types
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
print();

function foo0 () {
    print("jscall 0 and native call1");
}

function foo1 (a) {
    print("jscall 1 and native call2", a);
}

function foo2 (a, b) {
    print("jscall 2 and native call3" ,a ,b);
}

function foo3 (a, b, c) {
    print("jscall 3 and native callirange", a, b, c);
}

function foo4 (a, b, c, d) {
    print("jscallirange and native callirange", a, b, c, d);
}

var obj = {
    member : 1,
    bar: function () {
        print(this.member);
    },
    get type() {
        return this.member;
    },
    set type(n) {
        this.member = n;
        print("setter", this.member);
    }
}
foo0();
foo1(1);
foo2(1, 2);
foo3(1, 2, 3);
foo4(1, 2, 3, 4);
obj.bar();
if (Number.isNaN(5) == false) {
    print("native callithisrange");
}

function Foo(x) {
    print("newobj", x);
}
var a = new Foo(2);
print(a);
var b = new String("newobj native");
print(b);

obj.type = 555;
print("getter", obj.type);

function test_func7(f) {
    function t() {
        try {
            t();
        } catch (e) {
            return f();
        }
    }
    return t();
}
class c0 {
}
class c1 extends c0 {
}
function f1(v5, v6) {
    return new c1(v5, v6)
}

Object.defineProperty(Array.prototype, Symbol.iterator, {
    value: function* () { },
    configurable: true
});

try {
    var v1 = test_func7(() => {
        return f1(1, 2);
    });
    print(v1);
} catch (e) {
    print(e);
}

function prototypeCallTest(...arr) {
    print(this);
    print(arr);
}

prototypeCallTest.call(0);
prototypeCallTest.call(0, 1);
prototypeCallTest.call(0, 1, 2);
prototypeCallTest.call(0, 1, 2, 3);
prototypeCallTest.call(0, 1, 2, 3, 4);

var x = [];
x[new Number(0)] = 0;
print(x[0]);

const reg = /bar/;

reg[Symbol.match] = false;
print('/bar/'.startsWith(reg));
