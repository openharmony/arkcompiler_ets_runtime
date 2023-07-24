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
 * @tc.name:arrayjoin
 * @tc.desc:test Array.join
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var a = new Array(1).join("  ");
print(a.length);
var str1 = JSON.stringify(Array(3).join("0"));
print(str1);
var str2 = JSON.stringify(new Array(3).join("0"));
print(str2);
const arr = []
arr.length = 3
var str3 = JSON.stringify(arr.join("0"));
print(str3)

// test circular reference
var arr1 = [1];
arr1.push(arr1);
arr1.push(arr1);
print(arr1.toString());
print(arr1.toString());

var arr2 = [1];
var arr3 = [2];
arr2[10] = arr3;
arr3[10] = arr2;
print(arr2.toString());
print(arr2.toString());

var arr4 = [1];
var arr5 = [2];
var arr6 = [3];
arr4.push(arr5);
arr5.push(arr6);
arr6.push(arr4);
print(arr4.toString());
print(arr4.toString());