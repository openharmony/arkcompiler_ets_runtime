/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

(() => {
	let result;
  function TestObjectNull() {
    result = new Object(null);
  }

  function TestObjectUndefined() {
    result = new Object(undefined);
  }

  function TestObjectNumber() {
    result = new Object();
  }

  TestObjectNull();
  print(result)

  TestObjectUndefined();
  print(result)

  TestObjectNumber();
  print(result)

})();
