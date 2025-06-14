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

import { a, func, b } from './A.js';
let val = 0;
function T() {
  class Test {
      constructor(a) {
          this.a = a
      }
  }

  var test = new Test(a)
  func();
  val = test.a + b;
}

for (let i = 0; i < 10; i++) {
  T();
}

ArkTools.jitCompileAsync(T);
let res = ArkTools.waitJitCompileFinish(T);
print(res)
ArkTools.setHotReloadPatchMain();
T();
print(val)
