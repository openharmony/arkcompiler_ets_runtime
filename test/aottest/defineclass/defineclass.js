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

function f10(a11, a12, a13) {
  class C43 extends a11 {
    constructor(a45) {
    }
  }
  const o56 = {
    "b" : -5.32,
    ...undefined,
    set d(a55) {},
  }
}
print("compiler succ")

class C3 {
  static b;
}
Object.defineProperty(C3, "d", { value: 1.2 });
class C32 extends C3 {
}
print("compiler succ")
