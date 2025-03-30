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

declare function print(str:any):string;
declare var ArkTools:any;

let a = 2
function f3() {
    try {
      for (let i = 0; i < 1; i++) {
        print("Test Success")
      }
    } catch(e) {
    }
    a = 3
}

f3()
print(ArkTools.isAOTCompiled(f3));

function f4() {
  let v3 = 3;
  for (let i44 = 0; i44 < 5; i44++) {
      let v53 = 0;
      do {
          i44 = v3;
          try {
              i44.o(375);
          } catch (e) {}
          v53++;
      } while (v53 < 2)
      v3++;
  }
}
print(ArkTools.isAOTCompiled(f4));

class C0 {}
const v0 = new C0()
const v1 = new Uint16Array(128)
function f5() {
    for (const v in v1) {
        const v2 = new Uint16Array(128)
        try { v0.p(128) } catch (e) {}
        for (let i = v2; i; i--) {}
    }
}
f5()
print(ArkTools.isAOTCompiled(f5));