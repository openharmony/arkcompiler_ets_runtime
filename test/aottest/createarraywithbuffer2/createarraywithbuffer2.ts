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

function basePullAll(array) {
    return array;
}

function pullAll(array, values) {
    return (array && array.length && values && values.length)
      ? basePullAll(array, values)
      : array;
}

function pullAll_Test() {
    const source = ['a', 'b', 'c', 'a', 'b', 'c']
    let result = null;
    for (let i = 0; i < 10; i++) {
        result = pullAll(source, ['a', 'c']);
    }
    print(`done`)
}

function lodash_Array_Test() {
    pullAll_Test()
}

lodash_Array_Test()

let a = {x: 1, y: 2, z: 3}

let b = {x: 1, y: 2}
let c = {x: 1}

if (ArkTools.isAOTCompiled(foo)) {
    let d = {x: 1, y: 2, z: 3, u: 4}
    print(d.y)
}

function foo() {
    return a.x;
}

function result() {
    return false;
}

print(foo())
