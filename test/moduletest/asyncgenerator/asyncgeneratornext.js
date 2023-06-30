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
 * @tc.name:asyncgenerator
 * @tc.desc:test asyncgenerator function
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

async function *a() {
  print(1);
  let c = yield 2;
  print(c);
}

async function *f() {
  return 0;
}

let b = a();
print("asyncgenerator next start");
b.next();
b.next(3);

const g = f();
g.next();
g.next();
print("asyncgenerator double next");
print("asyncgenerator next end");