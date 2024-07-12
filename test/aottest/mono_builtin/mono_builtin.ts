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

declare function print(arg:any):string;

declare interface ArkTools {
  isAOTCompiled(args: any): boolean;
}

function foo(arr) {
	let length = arr.length;
	let sum = 0;
	for (let i = 0; i < length; i++) {
		sum += arr[i]
	}
	for (let i = 0; i < length; i++) {
		sum = sum * arr[i]
	}
	for (let i = 0; i < length; i++) {
		sum = sum * arr[i]
	}
}
	
let arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
const typedArray = new Int8Array(8);
typedArray[0] = 32;
foo(arr)
foo(typedArray)
print(ArkTools.isAOTCompiled(foo))
print(ArkTools.isAOTDeoptimized(foo))