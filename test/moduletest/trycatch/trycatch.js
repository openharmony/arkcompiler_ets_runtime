/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 * @tc.name:trycatch
 * @tc.desc:test try-catch
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
const o = {
}
o.toString = 'a'
const o11 = {
} 
try {
    o11[[o]] = []
} catch (error) {
    print("TypeError")
}

try {
    var a = 1;
    a();
} catch(e) {
    print(e.name);
}

try {
    var arr = [0];
    arr.length = 10000000000;
} catch(e) {
    print(e.name);
}

try {
    print(e.name);
} catch(e) {
    print(e.name);
}