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
 * @tc.name:arrayforeach
 * @tc.desc:test Array.forEach
 * @tc.type: FUNC
 * @tc.require: issueI69HXO
 */

let func = (item, index) => `${index}_${JSON.stringify(item)}`;

let array1 = [1, 2, 3, 4, 5];
array1.forEach((item, index) => {
    print(func(item, index));
});

let array2 = [1, 2, 3, 4];
array2.forEach((item, index) => {
    array2.pop();
    print(func(item, index));
});

let array3 = new Array(10);
array3.forEach((item, index) => {
    func(item, index);
});
print("test arrayforeach success!!!");

let array4 = new Array(1024);
array4.forEach((item, index) => {
    func(item, index);
});
print("test arrayforeach success!!!");

try {
    [].forEach(1.1);
} catch (e) {
    print("CallbackFn is not callable")
}

var callCnt1 = 0;

function callbackfn1(val, idx, obj)
{
  callCnt1++;
}
print(callCnt1)

var arr1 = [1, 2, , 4, 5];
arr1.forEach(callbackfn1);

var callCnt2 = 0;

function callbackfn2(val, idx, obj)
{
  arr2[1000] = 3;
  callCnt2++;
}

var arr2 = new Array(10);
arr2[1] = 1;
arr2[2] = 2;
arr2.forEach(callbackfn2);
print(callCnt2)

var callCnt3 = 0;

function callbackfn3(val, idx, obj)
{
  callCnt3++;
}

var arr3 = new Array(10);
arr3[1] = undefined;
arr3.forEach(callbackfn3);
print(callCnt3)