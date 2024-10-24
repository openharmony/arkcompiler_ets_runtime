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
 * @tc.name:arraytoreversed
 * @tc.desc:test Array.toReversed
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

const arr0 = [0,undefined, , 1];
print(arr0.indexOf(undefined));
const arr1 = arr0.toReversed();
print(arr0.indexOf(undefined));
print(arr1.indexOf(undefined));

const arr2 = new Array(1025);
for(let i = 0; i < 1025; i = i + 1)
    arr2[i] = i;
const arr3 = arr2.toReversed();

var arr5 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr6 = new Array();
for (let i = 0; i < 10; i++) arr6[i] = i;

var result1 = arr5.toReversed();
print(result1);
var result2 = arr6.toReversed();
print(result2);

const arrString = ["1","2", , "3"];
print(arrString.indexOf(undefined));
const arrString1 = arrString.toReversed();
print(arrString.indexOf(undefined));
print(arrString1.indexOf(undefined));