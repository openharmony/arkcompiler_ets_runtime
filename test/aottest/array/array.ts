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

declare function assert_equal(a: Object, b: Object):void;
let a = []
let l = a.push(1)
assert_equal(l, 1)
l = a.push(1, 2, 3, 4, 5)
assert_equal(l, 6)

for (let i = 0; i < 100; i++) {
  a.push(i)
}

let c = [1, 2, 3, 4]
a.push(...c)

assert_equal(a.length, 110)

let b = []
b.push(1, 2, 3, 4)
b.push(1, 2, 3)
b.push(1, 2)
b.push(1)
b.push()
assert_equal(Object.values(b), [1, 2, 3, 4, 1, 2, 3, 1, 2, 1])
assert_equal(b.length, 10)
