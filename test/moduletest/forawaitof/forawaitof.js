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
 * @tc.name:forawaitof
 * @tc.desc:test forawaitof
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
let rest;

let iterCount = 0;
async function fn() {
  for await ({...rest} of ["foo"]) {
    print(rest["0"] == "f");
    print(rest["1"] == "o");
    print(rest["2"] == "o");
    print(rest instanceof Object);
    iterCount += 1;
  }
}

let promise = fn();

promise.then(() => print(iterCount));
