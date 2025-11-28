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
 * @tc.name:string_wrapper_unrelated_class_toprimitive
 * @tc.desc:String wrapper to primitive detector test - unrelated class doesn't invalidate
 * @tc.type: FUNC
 */
let isValid = ArkTools.isStringWrapperToPrimitiveDetectorValid();
print(isValid);

class MyNotString {
 [Symbol.toPrimitive]() {
   return 'new';
  }
}

let x = new MyNotString();
let result = 'got ' + x;
print(result === 'got new');

// Setting the toPrimitive symbol in a class unrelated to string wrappers won't
// invalidate the protector.
isValid = ArkTools.isStringWrapperToPrimitiveDetectorValid();
print(isValid);

// Also creating normal string wrappers won't invalidate the protector.
new String("nothing");
isValid = ArkTools.isStringWrapperToPrimitiveDetectorValid();
print(isValid);
