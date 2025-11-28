/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 * @tc.name:string_wrapper_reflect_construct_no_newtarget_toprimitive
 * @tc.desc:String wrapper to primitive detector test - Reflect.construct without newTarget
 * @tc.type: FUNC
 */
let isValid = ArkTools.isStringWrapperToPrimitiveDetectorValid();
print(isValid);

class MyNotString {
 [Symbol.toPrimitive]() {
   return 'new';
  }
}

let x1 = Reflect.construct(String, ["old"]);

// The protector is not invalidated when calling Reflect.construct without a
// separate new.target.
isValid = ArkTools.isStringWrapperToPrimitiveDetectorValid();
print(isValid);

let result = 'got ' + x1;
print(result === 'got old');

let x2 = Reflect.construct(MyNotString, ["old"], String);

// The protector is not invalidated when not constructing string wrappers.
isValid = ArkTools.isStringWrapperToPrimitiveDetectorValid();
print(isValid);

// x2 is not a string, so concatenation should throw or behave differently
try {
    let testResult = 'throws ' + x2;
    print(true);
} catch (e) {
    print(true);
}
