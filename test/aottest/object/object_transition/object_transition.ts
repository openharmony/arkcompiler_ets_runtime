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

function TestSample()
{
    print("TestSample:");
    let arr: number[] = new Array(1025).fill(0);
    arr[1] = 1;
    print(arr[1]);
}

function TestArrayWithElementsAndProperties()
{
    print("TestArrayWithElementsAndProperties:");
    let arr: number[] = new Array(2048)
    arr[1] = 2;
    arr[3] = 4;
    arr.x1 = 1;
    arr.x2 = 2;
    arr.x3 = 3;
    arr.x4 = 4;
    arr.x5 = 5;
    arr.length = 2047;

    arr.fill("000");
    print(arr[1]);
    print(arr[3]);
    print(arr.x1);
    print(arr.x2);
    print(arr.x3);
    print(arr.x4);
    print(arr.x5);
    print(arr.length);
}

function TestFullArrayWithElementsAndProperties()
{
    print("TestFullArrayWithElementsAndProperties:");
    let arr: number[] = new Array(2048)
    arr.x1 = 1;
    arr.x2 = 2;
    arr.x3 = 3;
    arr.x4 = 4;
    arr.x5 = 5;
    for (let i: number = 0; i < 2048; i++) {
        arr[i] = "apple"
    }
    arr.length = 2047;
    arr.fill(0);
    for (let i: number = 0; i < 5; i++) {
        print(arr[i]);
    }
    for (let i: number = 2045; i < 2048; i++) {
        print(arr[i]); 
    }
    print(arr.apple);
    print(arr.x1);
    print(arr.x2);
    print(arr.x3);
    print(arr.x4);
    print(arr.x5);
    print(arr.length);
}

function TestShouldNotOptimizeAsFastElements()
{
    print("TestShouldNotOptimizeAsFastElements:");
    let arr: number[] = new Array(1025)
    arr.x1 = 1;
    arr.x2 = 2;
    arr.x3 = 3;
    arr.x4 = 4;
    arr.x5 = 5;
    for (let i: number = 0; i < 1025; i++) {
        arr[i] = "apple"
    }
    arr.length = 0;
    arr.fill(0);
    for (let i: number = 0; i < 5; i++) {
        print(arr[i]);
    }
    for (let i: number = 1020; i < 1025; i++) {
        print(arr[i]); 
    }
    print(arr.apple);
    print(arr.x1);
    print(arr.x2);
    print(arr.x3);
    print(arr.x4);
    print(arr.x5);
    print(arr.length);
}

function TestStringArrayWithElementsAndProperties()
{
    print("TestStringArrayWithElementsAndProperties:");
    let arr: string[] = new Array(1025)
    arr[1] = "apple"
    arr.apple = "b"

    arr.fill("ark");
    print(arr[0]);
    print(arr[1]);
    print(arr.apple);
}

function TestSpecialCase()
{
    print("TestSpecialCase:");
    let arr: number[] = new Array(1025);
    Object.defineProperty(arr, '0', {
        value: 42,
        writable: false,
    });
    arr.fill(1);
    print(arr[0]);
}

TestSample();
TestArrayWithElementsAndProperties();
TestFullArrayWithElementsAndProperties();
TestShouldNotOptimizeAsFastElements();
TestStringArrayWithElementsAndProperties();
try {
    TestSpecialCase();
} catch (e) {}