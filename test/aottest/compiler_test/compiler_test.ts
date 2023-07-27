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
declare function print(str:any):string;

// case1: CREATEOBJECTWITHEXCLUDEDKEYS
const o4 : {} = {
    get c() : {}{
        let {...v3} = t12()
    },
}

// case2: ProcessSnapshotConstantPool
var m = {
    get a() {
        function f() {
            return 1
        }
    },
    "a": "hello"
}

// case3: last label equal bcLast
function f() {}
function f1() {
    return
    try {
      f()
    } catch(e) {
    }
}

// case4: GetDeps
function f2() {
    try {
        return
    } catch(e15) {
    }
    while(1) {}
}

print("compiler success");