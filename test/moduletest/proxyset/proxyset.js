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
  const validator = {
    set(target, prop, value) {
      if (prop === 'age' && (typeof value !== 'number' || value < 0)) {
        throw new TypeError('Age must be a positive number');
      }
      target[prop] = value;
      return true;
    }
  };
  const user = new Proxy({}, validator);
  try {
    user.age  = -5; // TypeError 
  } catch(error) {
    assert_equal(error instanceof TypeError, true);
  }
}
test1();

// test2: trap is undefined, set property.
function test2()
{
  const target = { data: 'original' };
  const proxy = new Proxy(target, {});
  
  proxy.data  = 'new value';
  assert_equal(target.data, "new value");
}
test2();

// test3: trap returned false.
function test3()
{
  const strictProxy = new Proxy({}, {
    set(target, prop, value) {
      target[prop] = value;
      return false; 
    }
  });
  try {
    strictProxy.test  = 1; // TypeError
  } catch(error) {
    assert_equal(error instanceof TypeError, true);
  }
}
test3();

// test4: trap is undefined, defineProperty.
function test4()
{
  let v1 = {};
  let v2 = new Proxy({}, v1);
  v2.get = {};
  assert_equal(v2.get instanceof Object, true);
}
test4();

test_end();