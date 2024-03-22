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
 * @tc.name:arrayfindIndex
 * @tc.desc:test Array.findlastIndex
 * @tc.type: FUNC
 * @tc.require: issueI8FBM3
 */

(function() {
    var a = [0, 1,,3];
    var index = a.findLastIndex(function(val){
        return val === undefined;
    });
    print(index);
})();

var arr1 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr2 = new Array();
function testFunction(element, index, array) {
    if (index == 9) {
        array.length = 6;
    }
    return element < 1;
}
for (let i = 0; i < 10; i++) arr2[i] = i;

var result1 = arr1.findLastIndex(testFunction);
print(result1);
var result2 = arr2.findLastIndex(testFunction);
print(result2);

let arr3 = [1, 2, , 7, , undefined];
arr3.__proto__.push(9);
arr3.__proto__.push(9);
arr3.__proto__.push(9);
function fun1(element) {
    return element === 1;
}
print(arr3.findLastIndex(fun1));
arr3.__proto__.pop(9);
arr3.__proto__.pop(9);
arr3.__proto__.pop(9);

//slowPath
let arr4 = [1, 2, 3, 4, 5, 6, 7, 8];
function func2(element, index, arr) {
    return element === 6;
}
print(arr4.findLastIndex(func2));

//notFound
let arr5 = new Array(5, 2, 7, 9, 20);
function func3(element, index, arr) {
    arr.length = 3;
    return element === 100;
}
print(arr5.findLastIndex(func3));