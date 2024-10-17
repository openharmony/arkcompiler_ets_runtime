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
 * @tc.name:sharedic
 * @tc.desc:test sharedic
 * @tc.type: FUNC
 * @tc.require: issueI8QUU0
 */

// @ts-nocheck
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
 * @tc.name:sharedic
 * @tc.desc:test sharedic
 * @tc.type: FUNC
 * @tc.require: issueI8QUU0
 */

// @ts-nocheck
declare function print(str: any): string;

class SFoo {
  propString: string = "I'm simple sendable's propString";
  constructor() {
    "use sendable"
  }
  testic() {
    let a = this;
    for (var i: number = 0; i < 100; i++) {
      a.propString = "a" ;
    }
  }
  
}
let t = new SFoo();
t.testic(); // sharedfunction not enter ic

class SClass {
  str: string = "I'm simple sendable's propString";
  constructor() {
    "use sendable"
  }
}

let s = new SClass()
for (var i: number = 0; i < 100; i++) {
  s.str;
}

let sobj = new SClass();
for (let i = 0; i < 200; i++) {
}
sobj.__proto__; // not SetProtoCell
{
  let arr = new SendableBigUint64Array(5);
  arr[0]=1n;
  print(arr[0]);
}

print("shared ic over");