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

{
    let arr = [1, , 2, , 3];
    let res = arr.flatMap(x => [x]);
    print(JSON.stringify(res));
}
{
    let arr = new Array(3000);
    arr[200] = 1;
    let res = arr.flatMap(x => [x]);
    print(JSON.stringify(res));
}
{
    class MyArray extends Array {
        static get [Symbol.species]() {
            return this;
        }
    }
    const wannabe = new MyArray();
    const result = wannabe.flatMap(x => [x, x]);
    print(result instanceof MyArray);
}

var arr1 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr2 = new Array();
function testFunction(element, index, array) {
    if (index == 0) {
        array.length = 6;
    }
    return [element, element * element];
}
for (let i = 0; i < 10; i++) arr2[i] = i;

var result1 = arr1.flatMap(testFunction);
print(result1);
var result2 = arr2.flatMap(testFunction);
print(result2);

var arr3 = [0, 1, , , 4, , 6, 7, 8, 9];
arr3.__proto__.push(0);
arr3.__proto__.push(1);
arr3.__proto__.push(2);
arr3.__proto__.push(3);
function testFunction(element, index, array) {
    if (index == 0) {
        array.length = 6;
    }
    if (index < 3)
        return 1;
    else
        return [element, element * element];
}
var result3 = arr3.flatMap(testFunction);
print(result3);
arr3.__proto__.pop(0);
arr3.__proto__.pop(1);
arr3.__proto__.pop(2);
arr3.__proto__.pop(3);

let arr4 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var result4 = arr4.flatMap(testFunction);
print(result4);

let arr = [1,,,,,5];
let res = arr.flatMap((x)=>{
    let ret=[x,x+1];
    ret[105] = x+2;
    Object.defineProperty(ret,2000,{value:x+3});
    return ret;
})
print(res);
print(res.length)


