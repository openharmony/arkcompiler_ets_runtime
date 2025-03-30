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

function Test() {
  const v2 = new Float32Array(8);
  const v5 = new Int8Array(1024);
  const v8 = new Uint32Array(42);
  let v9 = 1;
  let v13 = 0;
  do {
    let v14;
    try { v14 = v2.forEach(1024, v2); } catch (e) {}
    v8.b = Uint32Array;
    let v16 = 90009230923;
    const v17 = v16--;
    ++v9;
    v14--;
    const v20 = 42 << v14;
    new Float32Array(v13);
    v20 > 8;
    function f23(a24, a25, a26) {
      try {
        const t0 = 42;
      } catch (e) {}
      return a26;
    }
    f23(1024, v17, 2147483648)
    v13++;
  } while (v13 < 9)
  v5[2530] = v2;
}

Test();
ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));
Test();
