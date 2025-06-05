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

// @ts-nocheck

// 多态
class Base {
  baseNum: number = 1
  constructor(num: number){
    this.baseNum = num
  }
  compute(): number {
    return this.baseNum
  }
}

class DeriveDouble extends Base {
  constructor(num: number){
    super(num);
  }
  compute() : number {
    return this.baseNum * 2
  }
}

class DerivedTripple extends Base {
  constructor(num: number){
    super(num);
  }
  compute() : number {
    return this.baseNum * 3
  }
}


function Polymorphism() {
  let count = 100000;
  let result: number[] = new Array();
  let result2: Base[] = new Array();
  let result3: Base[] = new Array();
  for (let i = 0; i < count; i++) {
    result.push(i);
    result2.push(new DeriveDouble(i));
    result3.push(new DerivedTripple(i));
  }
  for (let i = 0; i < count; i++) {
    if (result[i] == i) {
      result2[i].baseNum = result2[i].compute();
      result3[i].baseNum = result3[i].compute();
    }
  }
  let res:boolean = true
  for (let i = 0; i < count; i++) {
    if (result2[i].baseNum != i * 2 || result3[i].baseNum != i * 3) {
      res = false
    }
  }
  if (!res) {
    print("result is wrong")
  }
}
let loopCountForPreheat = 1;

for (let i = 0; i < loopCountForPreheat; i++) {
  Polymorphism()
}

ArkTools.waitAllJitCompileFinish();

Polymorphism()
