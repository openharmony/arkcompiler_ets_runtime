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

declare function print(str:any):string;
try {
    throw "Throw Test";
}
catch (ex) {
    print(ex);
}

try {
    throw 123;
}
catch (ex) {
    print(ex);
}

// throw an exception when superCall case1: supercallspread
function testError(message) {
    this.message = message || "";
  }
  
  testError.prototype.toString = function () {
    return "testError: " + this.message;
  };
  
  testError.thrower = (message) => {
    throw new testError(message);
  };

let thrown1 = new testError("throw from A1");
let caught1;

class A1 {
  constructor(name, value) {
    throw thrown1;
  }
};
class B1 extends A1{
  constructor(...b: ConstructorParameters<typeof A1>) {
    try {
      super(...b);
    } catch (err) {
      caught1 = err;
    }
  }
};

try {
  new B1("test1", 11);
} catch (_) {};
print(caught1);
print(thrown1);

// throw an exception when superCall case2: supercallthisrange
let thrown2 = new testError("throw from A2");
let caught2;
class A2 {
  constructor() {
    throw thrown2;
  }
};

class B2 extends A2 {
  constructor() {
    try {
      super();
    } catch (err) {
      caught2 = err;
    }
  }
};

try {
  new B2();
} catch (_) {};
print(caught2);
print(thrown2);

//aot: when open inline mode, internal error should be throw correctly
class B {
  p(arg) {
      throw new Error('internal error') // internal error
  }
}
const v1 = new B()
let x = v1.p
let y = [111]
try{
  Reflect.apply(x, "111", y); // inline mode, error shold be throw here
  [11] // createarraywithbuffer, not here
} catch (e) {
  print(e)
};