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
 * @tc.name:regexp
 * @tc.desc:test Regexp
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
try {
  var v0 = "";
  var v1 = "";
  for(var v2 = 0; v2 < 10000; ++v2) {
      v0 += v0 + v2 + v0 + v1 + v2 + "";
  }
} catch(e) {
  print(e);
}

function runNearStackLimit(f) { function t() { try { t(); } catch (e) { f(); } }; try { t(); } catch (e) { } }
let obj = { "name": "tom" };
let px = new Proxy(obj, {});
function f() {
    for (let i = 0; i < 100; i++) { };
    px.name;
    const o9 = {
        [px]() {
        },
    };
    runNearStackLimit(f);
}
f();
print("Exception Test End")
