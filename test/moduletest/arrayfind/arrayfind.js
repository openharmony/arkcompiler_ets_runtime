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
 * @tc.name:stringSlice
 * @tc.desc:test String.slice
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */


const array1 = [5, 12, 8, 130, 44];

const found = array1.find((element) => element > 10);
print(found);


function isPrime(element, index, array) {
	let start = 2;
	while (start <= Math.sqrt(element)) {
		if (element % start++ < 1) {
			return false;
		}
	}
       return element > 1;
}

print([4, 6, 8, 12].find(isPrime)); // undefined，未找到
print([4, 5, 8, 12].find(isPrime)); // 5
