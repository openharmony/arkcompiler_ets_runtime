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
(function StoreToSuper () {
  "use strict";
  class A {
    s() {
      super.bla = 10;
    }
  };

  let a = new A();
  (new A).s.call(a);
  assert_equal(10 == a.bla, true);

  try {
    (new A).s.call(undefined);
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof TypeError, true);
  }
  
  try {
    (new A).s.call(42);
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof TypeError, true);
  }

  try {
    (new A).s.call(null);
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof TypeError, true);
  }
  
  try {
    (new A).s.call("abc");
    assert_unreachable();
  } catch (error) {
    assert_equal(error instanceof TypeError, true);
  }
  
})();


(function LoadFromSuper () {
  "use strict";
  class A {
    s() {
      return super.bla;
    }
  };

  let a = new A();
  assert_equal(undefined == (new A).s.call(a), true);
  assert_equal(undefined == (new A).s.call(undefined), true);
  assert_equal(undefined == (new A).s.call(42), true);
  assert_equal(undefined == (new A).s.call(null), true);
  assert_equal(undefined == (new A).s.call("abc"), true);
})();

class TestA {
  constructor() {
    assert_equal("TestA "+this.constructor.name+" "+new.target.name, 'TestA TestC TestC');
  }
}
class TestB extends TestA {
  constructor() {
      super();
      assert_equal("TestB "+this.constructor.name+" "+new.target.name, 'TestB TestC TestC');
      this.test();
  }
}
class TestC {
  constructor() {
      ("TestC", this.constructor.name, new.target.name);
  }
  test() {
      assert_equal("TestC", 'TestC');
  }
}
let c1 = Reflect.construct(TestB, [], TestC.prototype.constructor);
let c2 = Reflect.construct(TestB, [], c1.constructor);

test_end();