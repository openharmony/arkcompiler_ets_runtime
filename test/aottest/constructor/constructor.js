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

(() => {
	let result;
  function TestObjectNull() {
    result = new Object(null);
  }

  function TestObjectUndefined() {
    result = new Object(undefined);
  }

  function TestObjectNumber() {
    result = new Object();
  }

  TestObjectNull();
  print(result)

  TestObjectUndefined();
  print(result)

  TestObjectNumber();
  print(result)

})();

let v0 = [1, 2, 3];

function f3() {
  return v0;
}

class C7 {
  constructor(a9, a10, a11) {
    a11 / a11;
    return v0;
  }
  static toString(a21, a22) {
    this["toString"] = a22;
    const v23 = "2Al" / f3;
    v0 &= a22;
    const v24 = new f3();
    Reflect.construct(C7, [], C7);
    return "toString";
  }
}

try {
  const v39 = new C7("toString", "toString", C7);
} catch(e) {
  print(e);
}

class C27 extends Uint8ClampedArray {
  constructor(a29) {
    super(a29);
    print("hello world");
  }
}
new C27(1);
function test(){}
try {
  Reflect.construct(C27, [-1], test);
} catch(e) {
  print(e);
}
