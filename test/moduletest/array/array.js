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

var arr7 = [];
arr7[2] = 10;
print(arr7.pop());  // 10
print(arr7.pop());  // undefined
print(arr7.pop());  // undefined

var arr8 = [];
arr8[1] = 8;
print(arr8.shift());  // undefined
print(arr8.shift());  // 8
print(arr8.shift());  // undefined

// Test on Array::Splice
var arr9 = new Array(9);
print(arr9.length); // 9
arr9.splice(0, 9);
print(arr9.length); // 0
for(let i = 0; i < arr9.length; i++) {
    print(arr9[i]);
}

var arr10 = new Array(9);
print(arr10.length); // 9
arr10.splice(0, 8, 1);
print(arr10.length); // 2
for(let i = 0; i < arr10.length; i++) {
    print(arr10[i]); // 1, undefined
}

var arr11 = new Array(9);
print(arr11.length); // 9
arr11.splice(1, 8, 1);
print(arr11.length); // 2
for(let i = 0; i < arr11.length; i++) {
    print(arr11[i]); // undefined, 1
}

var arr12 = new Array(9);
print(arr12.length); // 9
arr12.splice(0, 4, 1, 2, 3, 4, 5);
print(arr12.length); // 10
for(let i = 0; i < arr12.length; i++) {
    print(arr12[i]); // 1, 2, 3, 4, 5, undefined, undefined, undefined, undefined, undefined
}

var arr13 = new Array(9);
print(arr13.length); // 9
arr13.splice(7, 5, 1, 2, 3);
print(arr13.length); // 10
for (var i = 0; i < arr13.length; i++) {
    print(arr13[i]); // undefined, undefined, undefined, undefined, undefined, undefined, undefined, 1, 2, 3
}

var arr14 = Array.apply(null, Array(16));
print(arr14.length);
var arr15 = Array.apply(null, [1, 2, 3, 4, 5, 6]);
print(arr15.length);

var a = '0';
print(Array(5).indexOf(a));