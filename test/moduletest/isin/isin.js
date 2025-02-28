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

const typedArrayConstructors = [
    Float64Array,
    Float32Array,
    Int32Array,
    Int16Array,
    Int8Array,
    Uint32Array,
    Uint16Array,
    Uint8Array,
    Uint8ClampedArray
];

/*
 * @tc.name:isin
 * @tc.desc:test isin. Test hasproperty in the prototype chain. 
 *                     If element index is greater than the length of the typed array, return false.
 * @tc.type: FUNC
 */
function isInTypedArray1(ctor) {
    const typedArray = new ctor(5);
    typedArray[1] = 42;

    const obj = {};
    obj.__proto__ = typedArray;

    print(1 in obj); // True. Found in the prototype chain.
    print(obj[1]);   // 42
    print(2 in obj); // True. Found in the prototype chain.
    print(obj[2]);   // 0
    print(6 in obj); // False. Not found in the prototype chain.
}

/*
 * @tc.name:isin
 * @tc.desc:test isin. Test hasproperty in the prototype chain. If property key is not a symbol, and
 *                     obj is a typedArray, do not find the property in the prototype chain.
 * @tc.type: FUNC
 */
function isInTypedArray2(ctor) {
    let obj1 = {};
    obj1[6] = "b";

    const typedArray = new ctor(5);
    typedArray[1] = 42;
    typedArray.__proto__ = obj1;
    const obj = Object.create(typedArray);
    print("6" in obj);
}

typedArrayConstructors.forEach((ctor) => {
    isInTypedArray1(ctor);
    isInTypedArray2(ctor);
})

/*
 * @tc.name: isin
 * @tc.desc: test isin. Test whether the return value of IsIn is exception while input para is not a ECMA obj.
 * @tc.type: FUNC
 */
{
    try {
        1 in 0;
        let tmpArr = [0];
    } catch (e) {
        print(e);
    }
}
