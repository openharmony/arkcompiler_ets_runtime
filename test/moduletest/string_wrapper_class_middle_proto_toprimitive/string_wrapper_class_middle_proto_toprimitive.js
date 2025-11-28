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
 * @tc.name:string_wrapper_class_middle_proto_toprimitive
 * @tc.desc:String wrapper to primitive detector test - class with middle class __proto__
 * @tc.type: FUNC
 */
let isValid = ArkTools.isStringWrapperToPrimitiveDetectorValid();
print(isValid);

class MyMiddle {}

class MyString extends MyMiddle {
 [Symbol.toPrimitive]() {
   return 'new';
  }
}
MyMiddle.__proto__ = String.prototype;

isValid = ArkTools.isStringWrapperToPrimitiveDetectorValid();
print(isValid);

let x = new MyString('old');
let result = 'got ' + x;
print(result === 'got new');
