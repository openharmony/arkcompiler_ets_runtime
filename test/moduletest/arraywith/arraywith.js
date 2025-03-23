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
 * @tc.name:arraywith
 * @tc.desc:test Array.with
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

const arr0 = [0, , undefined, 1];
print(arr0.indexOf(undefined));
const arr1 = arr0.with(0, 0);
print(arr0.indexOf(undefined));
print(arr1.indexOf(undefined));

const arr2 = new Array(1025);
for(let i = 0; i < 1025; i = i + 1)
    arr2[i] = i;
const arr3 = arr2.with(0, 0);

var arr4 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr5 = new Array();
for (let i = 0; i < 10; i++) arr5[i] = i;

var result1 = arr4.with(4, 100);
print(result1);
var result2 = arr5.with(4, 100);
print(result2);

let outputs = []
// testCase1: use only one parameter in Array.prototype.with()
const caseOneArr1 = [0, 1, 2, 3, 4, 5, , 7, 8, undefined];
outputs.push(caseOneArr1.indexOf(undefined));
const caseOneArr2 = caseOneArr1.with(2);
outputs.push(caseOneArr2.indexOf(undefined));

// testCase2: mixed-type array
const caseTwoArray1 = [1, "two", 3, "four"];
const caseTwoArray2 = caseTwoArray1.with(2, "three");
outputs.push(caseTwoArray2);

// testCase3: replace undefined or null
const caseThreeArray1 = [undefined, null, 3, 4];
const caseThreeArray2 = caseThreeArray1.with(0, 1);
const caseThreeArray3 = caseThreeArray1.with(1, "two");
outputs.push(caseThreeArray2);
outputs.push(caseThreeArray3);

// testCase4: replace by boolean, object or function
const caseFourArray1 = [1, "two", 3, "four"];
const caseFourArray2 = caseFourArray1.with(0, false);
const caseFourArray3 = caseFourArray1.with(1, { key: "value" });
const caseFourArray4 = caseFourArray1.with(2, () => "world");
outputs.push(caseFourArray2);
outputs.push(caseFourArray3);
outputs.push(caseFourArray4);

// testCase5: deal with sparse array
const caseFiveArray1 = [1, , 3, , 5];
const caseFiveArray2 = caseFiveArray1.with(1, "two");
outputs.push(caseFiveArray2);

// testCase6: deal with indices that are negative or out of bounds
const caseSixArray1 = [1, 2, 3, 4];
const caseSixArray2 = caseSixArray1.with(-1, 2);
outputs.push(caseSixArray2);
try {
    print(caseSixArray1.with(-6, 2));
} catch (error) {
    print(error);
}

print(outputs.map(out => JSON.stringify(out)).join('\n'));
