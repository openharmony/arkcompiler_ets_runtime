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
delete Float64Array.prototype.__proto__[Symbol.iterator];
let a = new Float64Array(9);
Object.defineProperty(a, "length", {
  get: function () { 
	ArkTools.arrayBufferDetach(a.buffer); 
	return 6; 
  }
});

print(Float64Array.from(a).length);