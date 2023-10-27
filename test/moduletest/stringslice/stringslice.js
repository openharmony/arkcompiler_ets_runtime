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
 * @tc.name:stringSlice
 * @tc.desc:test String.slice
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

let str = "The morning is upon us."; // str1 的长度是 23。
print(str.slice(1, 8));
print(str.slice(4, -2));
print(str.slice(12));
print(str.slice(30));
print(str.slice(-3));
print(str.slice(-3, -1));
print(str.slice(0, -1));
print(str.slice(4, -1));
print(str.slice(-11, 16));
print(str.slice(11, -7));
print(str.slice(-5, -1));