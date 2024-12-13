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

/*
 * @tc.name:sendabletypedarray
 * @tc.desc:test sendabletypedarray
 * @tc.type: FUNC
 * @tc.require: issue#I9EBOZ
 */

// @ts-nocheck
declare function print(str: any): string;
declare function isSendable(obj: lang.ISendable | Object): boolean;

let a = 0;
function f(b) {
  a += b;
}
for (let i = 0; i < 10000; i++) {
  f(0);
}

class SClass {
  str: string | undefined;
  constructor(s: string) {
    "use sendable"
    this.str = s;
    print(ArkTools.isTreeString(s));
    print(ArkTools.isTreeString(this.str));
    print(s === this.str);
  }
}

let s1 = "aaaaaaaaaaaaaaaaaaaaaa";
let s2 = "bbbbbbbbbbbbbbbbbbbbbb";
let s = s1 + s2;

let obj = new SClass(s);
for (let i = 0; i < 2; i++) {
  obj.str = s;
  print(ArkTools.isTreeString(s));
  print(ArkTools.isTreeString(obj.str));
  print(s === obj.str);
}
s = "abc";
for (let i = 0; i < 2; i++) {
  obj.str = s;
  print(s === obj.str);
}