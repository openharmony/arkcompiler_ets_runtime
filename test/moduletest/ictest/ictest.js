/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

// enable ic
var a = 0
for (var i = 0; i < 10000; i++) {
    a = a + a;
}

// case 1
// test toString in holder.
function t() {}
var I = t.prototype
I.toInt = function() {}
I.toNumber = function() {}
I.toString = function() {}
print(Object.getOwnPropertyDescriptor(I, 'toString').enumerable)

const o6 = {
  ..."function",
}
print(Object.getOwnPropertyDescriptor(o6, 0).configurable)

try {
  const v2 = ("string").match(ArrayBuffer)
  class C4 extends Array {
    constructor(a6, a7) {
      super()
      try {
        this.concat(ArrayBuffer, v2, ArrayBuffer);
        return v2
      } catch (e) {}
    }
  }
  new C4(C4, C4);
} catch(e) {}