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
 * @tc.name:array
 * @tc.desc:test Array
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var arr = new Array(100);
var a = arr.slice(90, 100);
print(a.length);

var arr1 = [1,1,1,1,1,1];
arr1.fill(0, 2, 4);
print(arr1);

var arr2 = new Array(100);
arr2.fill(0, 2, 4);
var a1 = arr2.slice(0, 5);
print(arr2.length);
print(a1);

var arr3 = [1,2,3,4,5,6];
arr3.pop();
print(arr3.length);
print(arr3);

var arr4 = [1,3,4];
arr4.splice(1, 0, 2);
print(arr4.length);
print(arr4);
// 1, 2, 3, 4
var arr4 = [1,2,3,4];
arr4.splice(3, 1, 3);
print(arr4.length);
print(arr4);
// 1, 2, 3, 3

var arr5 = [1,2,3,4,5,6];
arr5.shift();
print(arr5.length);
print(arr5);
// 1, 2, 3, 4, 5

var arr6 = [1,2,3,4,5];
arr6.reverse();
print(arr6);

var arr7 = Array.apply(null, Array(16));
print(arr7.length);
var arr8 = Array.apply(null, [1, 2, 3, 4, 5, 6]);
print(arr8.length);