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

// @ts-nocheck
declare function print(str:any):string;

class A {
  private b: null[]
  private a: number = 1
  constructor() {
    this.b.push(null);
  }
  f() {
    this.b[this.a--] = null;
  }
}
print("compiler succ")

const v1 = [-10];
const v2 = [v1];
v2[0] = -1.1;
print("compiler succ")
