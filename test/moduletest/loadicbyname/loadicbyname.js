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

/*
 * @tc.name:loadicbyname
 * @tc.desc:test loadicbyname
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
class C14 {
  constructor() {
      function F21() {
          return 0x4242424242424242n;
      }
      function F26() {
          try {
              this.toString();
          } catch(e30) {
          }
      }
      const v35 = new F26();
      const v38 = v35.constructor;
      v38.prototype = 0x4141414141414141n;
      Object.defineProperty(v38, 0x4343434343434343n, { writable: true, configurable: true, value: F21 });
      new v38(F26);
  }
}

new C14();
let flag1 = false;
try {
  new C14();
} catch (e) {
  flag1 = true;
}
print(flag1);