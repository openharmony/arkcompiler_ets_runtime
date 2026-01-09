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
 * @tc.name:Symbol
 * @tc.desc:test Symbol
 * @tc.type: FUNC
 * @tc.require: issueI7EPY3
 */

const p = Symbol.prototype;
try {
    p.toString();
} catch (e) {
    print("test Symbol.prototype.toString successful !!!");
}

try {
    p.valueOf();
} catch (e) {
    print("test Symbol.prototype.valueOf successful !!!");
}
print(JSON.stringify(Array.prototype[Symbol.unscopables]));

print(Symbol('foo').toString());

print(Symbol.for("foo").toString());

{
    let syArr = [];
    for (let i = 0; i < 100; i++) {
        syArr.push(Symbol());
    }
    let hashSet = new Set();
    for (let i = 0; i < 100; i++) {
        hashSet.add(ArkTools.hashCode(syArr[i]));
    }
    print(hashSet.size)
}
