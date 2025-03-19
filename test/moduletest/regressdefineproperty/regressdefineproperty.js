/*
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
 */
let a = [];
Object.defineProperty(a, "length", {writable: false});
function f() {
  return a.pop();
}
try{
    f();
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError, true)
}

function f1() {
    let x = [0,0,0,0,0];
    Object.defineProperty(x, 'length', {value : 4, enumerable : true});
}
try{
    f1();
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError, true)
}

let array = [];
Object.defineProperty(array, 'length', {writable: false});
assert_equal(array.length, 0);
try {
    array.shift()
    assert_unreachable();
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError, true)
}
let object = { length: 0 };
Object.defineProperty(object, 'length', {writable: false});
assert_equal(object.length, 0);
try {
    Array.prototype.shift.call(object)
    assert_unreachable();
} catch {
    var str = "true";
    assert_equal(str, "true")
}

Object.defineProperty(this, 'x', {
  configurable: true,
  get: function () {return 100}
});
Object.defineProperty(this, 'x', {
  value: 10
});
assert_equal(JSON.stringify(Object.getOwnPropertyDescriptor(this, 'x')), '{"value":10,"writable":false,"enumerable":false,"configurable":true}');

const o1 = {
  k: 1
};
for (let i = 0; i < 1100; i++) {
  Object.defineProperty(o1, "k" + i, {
    value: 0,
    enumerable: false
  });
}
assert_equal(JSON.stringify(o1), '{"k":1}')

function fn() { };
let v0 = function fn1() { }.bind(fn);
Object.defineProperty(v0, "length", {
  writable: true
})
v0.length = 42;
assert_equal(v0.length, 42)

test_end();