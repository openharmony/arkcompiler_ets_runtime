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
 * @tc.name:sendableenv
 * @tc.desc:test sendableenv
 * @tc.type: FUNC
 * @tc.require: issueI8R6LC
 */

// @ts-nocheck
declare function print(str: any): string;
class A {
  constructor() {
    "use sendable";
  }
  a:number = 1;
  static c:number = 2;
}

class B {
  constructor() {
    "use sendable";
  }
  b:A = new A();
  d:number = A.c;
}

let sObj = new B();
print("sObj A:a :" + sObj.b.a);
print("sObj A:c :" + sObj.d);

class C {
  public num: number = 0;
  constructor(num: number) {
      "use sendable"
      this.num = num;
  }
  PrintNum() {
    print("c.num :" + this.num);
  }
}

function testObjLiteral() {
 let b = { toString() {} };
 let c = new C(5);
 c.PrintNum();
}

testObjLiteral()