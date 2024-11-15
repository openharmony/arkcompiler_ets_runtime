/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 * @tc.name:arraystablecheck1
 * @tc.desc:test array stable or not
 */

{
    let arr1 = new Array(1023);
    let arr2 = new Array(1023);
    print(ArkTools.isStableJsArray(arr1));
    print(ArkTools.isStableJsArray(arr2));
    print(ArkTools.isStableJsArray(arr2.__proto__));
}


/*
 * @tc.name:arraystablecheck2
 * @tc.desc:test array stable or not
 */

{
    let arr1 = new Array(1023);
    let arr2 = new Array(1023);
    arr1.__proto__ = arr2;
    print(ArkTools.isStableJsArray(arr1));
    print(ArkTools.isStableJsArray(arr2));
}

/*
 * @tc.name:arraystablecheck3
 * @tc.desc:test array stable or not
 */
{
    let arr1 = new Array(1026);
    let arr2 = new Array(1026);
    print(ArkTools.isStableJsArray(arr1));
    print(ArkTools.isStableJsArray(arr2));
}

/*
 * @tc.name:arraystablecheck4
 * @tc.desc:test array stable or not
 */
{
    class MyArray extends Array {

    };
    let arr1 = new MyArray(1023);
    let arr2 = new MyArray(1023);
    let arr3 = arr1.concat(arr2);
    let arr4 = new Array(1023);
    print(ArkTools.isStableJsArray(arr1));
    print(ArkTools.isStableJsArray(arr2));
    print(ArkTools.isStableJsArray(arr3));
    print(ArkTools.isStableJsArray(arr4));
}

/*
 * @tc.name:arraystablecheck5
 * @tc.desc:test array stable or not
 */
{
    class MyArray extends Array {

    };
    let arr1 = new MyArray();
    let arr2 = new MyArray();
    let arr3 = arr1.concat();
    let arr4 = new Array();
    print(ArkTools.isStableJsArray(arr1));
    print(ArkTools.isStableJsArray(arr2));
    print(ArkTools.isStableJsArray(arr3));
    print(ArkTools.isStableJsArray(arr4));
}

/*
 * @tc.name:arraystablecheck6
 * @tc.desc:test array stable or not
 */
{
    class MyArray extends Array {

    };
    let arr1 = new MyArray(1, 2, 3, 4, 5);
    let arr2 = new MyArray(1, 2, 3, 4, 5);
    let arr3 = arr1.concat();
    let arr4 = new Array(1, 2, 3, 4, 5);
    print(ArkTools.isStableJsArray(arr1));
    print(ArkTools.isStableJsArray(arr2));
    print(ArkTools.isStableJsArray(arr3));
    print(ArkTools.isStableJsArray(arr4));
}

/*
 * @tc.name:arraystablecheck7
 * @tc.desc:test array stable or not
 */
{
    let arr1 = new Array(1023);
    print(ArkTools.isStableJsArray(arr1));
    arr1.__proto__.fill(233, 0, 0);
    print(ArkTools.isStableJsArray(arr1));
    arr1.__proto__.fill();
    print(ArkTools.isStableJsArray(arr1));
    arr1.__proto__.push();
    print(ArkTools.isStableJsArray(arr1));
    arr1.__proto__.unshift();
    print(ArkTools.isStableJsArray(arr1));
    arr1.__proto__.splice();
    print(ArkTools.isStableJsArray(arr1));
}

/*
 * @tc.name:arraystablecheck8
 * @tc.desc:test array stable or not
 */
{
    let arr1 = new Array(1023);
    let arr2 = new Array(1023);
    arr1.__proto__.push(1);
    print(ArkTools.isStableJsArray(arr1.__proto__));
    print(ArkTools.isStableJsArray(arr1));
    print(ArkTools.isStableJsArray(arr2));
}