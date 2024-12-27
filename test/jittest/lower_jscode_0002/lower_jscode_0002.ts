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

function assert(mustBeTrue, message) {
  if (mustBeTrue == true) {
    return;
  }
  if (message == undefined) {
    message = 'Expected true but got ' + assert._toString(mustBeTrue);
  }
  throw new Error(message);
}

function isEnumerable(obj, name) {
  var stringCheck = false;
  if (typeof name == "string") {
    for (var x in obj) {
      if (x == name) {
        stringCheck = true;
        break;
      }
    }
  } else {
    stringCheck = true;
  }
  return stringCheck &&
    Object.prototype.hasOwnProperty.call(obj, name) &&
    object.prototype.propertyIsEnumerable.call(obj, name);
}

function verifyNotEnumerable(obj, name) {
  assert(!Object.getOwnPropertyDescriptor(obj, name).enumerable,
    "Expected obj[" + String(name) + "] to have enumerbale:false.");
  if (isEnumerable(obj, name)) {
    throw new Error("Error");
  }
  return true;
}



verifyNotEnumerable(Array, 'name');
ArkTools.jitCompileAsync(verifyNotEnumerable);
print(ArkTools.waitJitCompileFinish(verifyNotEnumerable));
verifyNotEnumerable(Array, 'name');
