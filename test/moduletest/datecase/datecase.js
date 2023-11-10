/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
 * @tc.name:datecase
 * @tc.desc:test date
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var d1 = new Date(2022, 0, 1);
var d2 = new Date(2022, 1, 2);
var d3 = new Date(2022, 2, 3);
var d4 = new Date(2022, 3, 4);
var d5 = new Date(1677628800000);
var d6 = new Date(1680278400000);

var d9 = new Date(1970, 1, 1);
var d10 = new Date(2022, 2, 26, 23, 59); // dst become effective in GMT+02:00

print(d1.getFullYear() == 2022);
print(d1.getMonth() == 0);
print(d1.getDate() == 1);

print(d2.getFullYear() == 2022);
print(d2.getMonth() == 1);
print(d2.getDate() == 2);

print(d3.getFullYear() == 2022);
print(d3.getMonth() == 2);
print(d3.getDate() == 3);

print(d4.getFullYear() == 2022);
print(d4.getMonth() == 3);
print(d4.getDate() == 4);

print(d5.getFullYear() == 2023);
print(d5.getMonth() == 2);

print(d6.getFullYear() == 2023);
print(d6.getMonth() == 3);

print(d9.getFullYear() == 1970);
print(d9.getMonth() == 1);
print(d9.getDate() == 1);
print(d9.getHours() == 0);

print(d10.getMonth(),d10.getDate(),d10.getHours());