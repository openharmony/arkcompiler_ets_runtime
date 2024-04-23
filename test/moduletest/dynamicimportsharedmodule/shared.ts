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
 * @tc.name:dynamicimportsharedmodule
 * @tc.desc:test dynamicimportsharedmodule
 * @tc.type: FUNC
 * @tc.require: issue#I9IFCJ
 */

declare function print(str: any): string;
"shared module"
export {SingleCase};

import("./func").then(m=>{
    let tA = new m.Test1("Test1");
    print(tA.foo());
})

class SingleCase {
  private static sin : SingleCase;
  private count : number = 0;
  private constructor() {
    "use sendable";
    print("SingleCase constructor");
  }
  public static getInstance(): SingleCase {
    if(!SingleCase.sin) {
        SingleCase.sin = new SingleCase();
    }
    return SingleCase.sin;
  }
  public incrementCount():number {
    return this.count++;
  }
}
