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

/*
 * @tc.name:asmstackoverflow
 * @tc.desc:test stack overflow in asm
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
function foo(x, y, z, a, b) {
    foo(x, y, z, a, b)
}
try {
    foo(1, 2, 3, 4, 5)
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

function foo2() {
    foo2()
}
try {
    foo2()
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

const obj = {};
const pro = new Proxy({}, obj);
obj.__proto__ = pro;
try {
    obj[10];
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}
try {
    function func() { }
    Function.prototype.__proto__ = new Proxy(func, {})
    Function.prototype.__proto__ = new Proxy(func, {})
	assert_unreachable();
} catch (error) {
    assert_equal(error instanceof RangeError, true);
}

// callarg0-3 callrange
function callarg0() {
    callarg0();
}
try {
    callarg0();
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

function callarg1(a) {
    callarg1(a);
}
try {
    callarg1(1);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

function callarg2(a, b) {
    callarg2(a, b);
}
try {
    callarg2(1, 2);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

function callarg3(a, b, c) {
    callarg3(a, b, c);
}
try {
    callarg3(1, 2, 3);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

function callrange(a, b, c, d) {
    callrange(a, b, c, d);
}
try {
    callrange(1, 2, 3, 4);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

// callthis0-3 callthisrange
var obj2 = {
    callthis0: function () {
        this.callthis0();
    },
    callthis1: function (a) {
        this.callthis1(a);
    },
    callthis2: function (a, b) {
        this.callthis2(a, b);
    },
    callthis3: function (a, b, c) {
        this.callthis3(a, b, c);
    },
    callthisrange: function (a, b, c, d) {
        this.callthisrange(a, b, c, d);
    },
}
try {
    obj2.callthis0();
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}
try {
    obj2.callthis1(1);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}
try {
    obj2.callthis2(1, 2);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}
try {
    obj2.callthis3(1, 2, 3);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}
try {
    obj2.callthisrange(1, 2, 3, 4);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

// callstatic
class MyClass {
    static callstatic0() {
        MyClass.callstatic0();
    }
    static callstatic1(a) {
        MyClass.callstatic1(a);
    }
    static callstatic2(a, b) {
        MyClass.callstatic2(a, b);
    }
    static callstatic3(a, b, c) {
        MyClass.callstatic3(a, b, c);
    }
    static callstaticrange(a, b, c, d) {
        MyClass.callstaticrange(a, b, c, d);
    }
}
try {
    MyClass.callstatic0();
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}
try {
    MyClass.callstatic1(1);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}
try {
    MyClass.callstatic2(1, 2);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}
try {
    MyClass.callstatic3(1, 2, 3);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}
try {
    MyClass.callstaticrange(1, 2, 3, 4);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

// callnew
class MyClass2 {
    constructor() {
        new MyClass2();
    }
}
try {
    new MyClass2();
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

class MyClass3 {
    constructor(a, b, c, d) {
        new MyClass3(a, b, c, d);
    }
}
try {
    new MyClass3(1, 2, 3, 4);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

function callarg0_with_callthis() {
    const arr = [1]
    function bar() {
        return 1;
    }
    arr.filter(bar);
    return foo_arg0();
}

try {
    foo_arg0();
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof ReferenceError, true);
}

function callarg1_with_callthis(a) {
    const arr = [1]
    function bar() {
        return 1;
    }
    arr.filter(bar);
    return callarg1_with_callthis(a);
}

try {
    callarg1_with_callthis(1);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

function callarg2_with_callthis(a, b) {
    const arr = [1]
    function bar() {
        return 1;
    }
    arr.filter(bar);
    return callarg2_with_callthis(a, b);
}

try {
    callarg2_with_callthis(1, 2);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

function callarg3_with_callthis(a, b, c) {
    const arr = [1]
    function bar() {
        return 1;
    }
    arr.filter(bar);
    return callarg3_with_callthis(a, b, c);
}

try {
    callarg3_with_callthis(1, 2, 3);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

function callrange_with_callthis(a, b, c, d) {
    const arr = [1]
    function bar() {
        return 1;
    }
    arr.filter(bar);
    return callrange_with_callthis(a, b, c, d);
}

try {
    callrange_with_callthis(1, 2, 3, 4);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof RangeError, true);
}

// PoC
var source = Array(2500).join("(") + "a" + Array(2500).join(")");
try {
    var r = RegExp(source);
    r.test("\x80");
	assert_unreachable();
} catch (e) {
    assert_equal(e.name, "SyntaxError");
}

// Poc
try {
    class MyClass4 {
        set f(a) {
            this.f = a;
        }
    }
    const instance4 = new MyClass4();
    instance4.f = false;
} catch (e) {
    print(e.name);
}
try {
    var arr = [1];
    arr.push(arr);
    const v7 = arr.flat(65535);
	assert_unreachable();
} catch (error) {
    assert_equal(error.name, "RangeError");
}

function f0() {
    return f0;
}
class C1 extends f0 {
    constructor(a3) {
        super();
        const v6 = new Proxy(C1, Reflect);
        new v6();
    }
}
try {
    new C1(f0);
    assert_unreachable();
} catch(e) {
    assert_equal(e.name, "RangeError");
}

const v3 = new Proxy([123], {});
class C4{
  constructor(a6, a7, a8) {
    try {
      new C4();
    } catch (e) {
        
    }
    new Set(v3);
  }
}
new C4();
const result1 = "recursive stack overflow"
assert_equal(result1, "recursive stack overflow")

function f(a5) {
    ("ZU").matchAll(a5);
}
function runNearStackLimit(f) {
    function t() {
        try {
            t();
        } catch (e) {
            f();
        }
    }
    try {
        t();
    } catch (e) {
    }
}
runNearStackLimit(f);
const result2 = "matchAll success";
assert_equal(result2, "matchAll success");

test_end();