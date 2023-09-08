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
 * @tc.name:builtinsobjectaddproperty
 * @tc.desc:test builtinsobject addproperty
 * @tc.type: FUNC
 * @tc.require: 
 */
print("builtins object add property begin");
var object = {
    "aa": "aa",
    "bb": "bb",
    "cc": "cc",
    "dd": "dd",
    "ee": "ee",
    "ff": "ff"
}

var str = "g";
for (var i = 0; i < 1100; i++) {
    object[str + i] = str;
}

print(Reflect.ownKeys(object).length);
// Expected output: 1106
print("builtins object add property end");