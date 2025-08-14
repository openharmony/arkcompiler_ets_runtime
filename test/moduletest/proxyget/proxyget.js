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

// test1: normal trap.
function test1()
{
  const target = { price: 99 };
  const handler = {
    get(target, prop) {
      return prop === 'price' ? `$${target[prop]}` : target[prop];
    }
  };
  const proxy = new Proxy(target, handler);
  
  assert_equal(proxy.price, "$99");  // "$99"
}
test1();

// test2: handle is not callable, throw TypeError.
function test2()
{
  try {
    const invalidHandler = { get: 'not a function' };
    const proxy = new Proxy({}, invalidHandler); // throw TypeError
    assert_equal(proxy.price, undefined);
  } catch (error) {
    assert_equal(error instanceof TypeError, true); // true
  }
}
test2();

// test3: CheckGetTrapResult, check data.
function test3()
{
  const target = {};
  Object.defineProperty(target, 'key', {
    value: 100,
    writable: false,
    configurable: false 
  });
  const proxy = new Proxy(target, {
    get(target, prop) {
      return 200;
    }
  });
  try {
    proxy.key;
  } catch (error) {
    assert_equal(error instanceof TypeError, true); // true
  }
}
test3();

// test4: CheckGetTrapResult, check accessor.
function test4()
{
  const target = {};
  Object.defineProperty(target, 'key', {
    set() {
    },
    configurable: false 
  });
  const proxy = new Proxy(target, {
    get(target, prop) {
      return 200;
    }
  });
  try {
    proxy.key;
  } catch (error) {
    assert_equal(error instanceof TypeError, true); // true
  }
}
test4();

// test5: CheckGetTrapResult, check accessor, configurable is true.
function test5()
{
  const target = {};
  Object.defineProperty(target, 'key', {
    set() {
    },
    configurable: true,
  });
  const proxy = new Proxy(target, {
    get(target, prop) {
      return 200;
    }
  });
  try {
    proxy.key;
  } catch (error) {
    assert_equal(error instanceof TypeError, false); // false
  }
}
test5();

// test6: CheckGetTrapResult, check accessor, trap result is undefined.
function test6()
{
  const target = {};
  Object.defineProperty(target, 'key', {
    set() {
    },
    configurable: false,
  });
  const proxy = new Proxy(target, {});
  try {
    proxy.key;
  } catch (error) {
    assert_equal(error instanceof TypeError, false); // false
  }
}
test6();

// test7: CheckGetTrapResult, check internal accessor.
function test7()
{
  const target = function f0(){};
  Object.defineProperty(target, 'length', {
    value: 0,
    configurable: false,
    writable: false,
  });
  const proxy = new Proxy(target, {
    get(target, prop) {
      return 200;
    }
  });
  try {
    proxy.length;
  } catch (error) {
    assert_equal(error instanceof TypeError, true); // true
  }
}
test7();

// test8: CheckGetTrapResult, check internal accessor, value is same.
function test8()
{
  const target = function f0(){};
  Object.defineProperty(target, 'length', {
    value: 200,
    configurable: false,
    writable: false,
  });
  const proxy = new Proxy(target, {
    get(target, prop) {
      return 200;
    }
  });
  try {
    proxy.length;
  } catch (error) {
    assert_equal(error instanceof TypeError, false); // false
  }
}
test8();

// test9: CheckGetTrapResult, check internal accessor, configurable is true.
function test9()
{
  const target = function f0(){};
  Object.defineProperty(target, 'length', {
    value: 200,
  });
  const proxy = new Proxy(target, {
    get(target, prop) {
      return 200;
    }
  });
  try {
    proxy.length;
  } catch (error) {
    assert_equal(error instanceof TypeError, false); // false
  }
}
test9();

function test10()
{
  let target = { a: 1 };
  const handler = {};
  handler.__proto__ = new Proxy(target, handler);
  try {
    handler.__proto__ = new Proxy(target, handler);
  } catch (error) {
    assert_equal(error instanceof RangeError, true);
    print(error);
  }
}
test10();

test_end();
