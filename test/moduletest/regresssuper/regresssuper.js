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
  print(10 == a.bla);

  try {
    (new A).s.call(undefined);
  } catch (error) {
    print(error instanceof TypeError);
  }
  
  try {
    (new A).s.call(42);
  } catch (error) {
    print(error instanceof TypeError);
  }

  try {
    (new A).s.call(null);
  } catch (error) {
    print(error instanceof TypeError);
  }
  
  try {
    (new A).s.call("abc");
  } catch (error) {
    print(error instanceof TypeError);
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
  print(undefined == (new A).s.call(a));
  print(undefined == (new A).s.call(undefined));
  print(undefined == (new A).s.call(42));
  print(undefined == (new A).s.call(null));
  print(undefined == (new A).s.call("abc"));
})();

class TestA {
  constructor() {
      print("TestA", this.constructor.name, new.target.name);
  }
}
class TestB extends TestA {
  constructor() {
      super();
      print("TestB", this.constructor.name, new.target.name);
      this.test();
  }
}
class TestC {
  constructor() {
      print("TestC", this.constructor.name, new.target.name);
  }
  test() {
      print("TestC");
  }
}
let c1 = Reflect.construct(TestB, [], TestC.prototype.constructor);
let c2 = Reflect.construct(TestB, [], c1.constructor);
