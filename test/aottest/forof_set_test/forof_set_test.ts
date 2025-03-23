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

// Test basic for of set.entries()
function func1() {
    let set = new Set([1, 2]); 
    for (let ele of set) {
        print(ele);
    }
}

// Test basic for of set.keys()
function func2() {
    let set= new Set(['a', 'b', 2]); 
    for (let ele of set.keys()) {
        print(ele);
    }
}

// Test basic for of set.values()
function func3() {
    let set = new Set(['a', 1, 'b', 2]); 
    for (let ele of set.values()) {
        print(ele);
    }
}

// Test update set 
function func4() {
    const set = new Set([1, 'a', 2, 'b', 3, 'c']);
  
    const iterator = Set.prototype[Symbol.iterator].call(set);
  
    for (let iterResult = iterator.next(); !iterResult.done; iterResult = iterator.next()) {
        print(iterResult.value);
        if (iterResult.value == 2) {
            set.delete('b');
        }

        if (iterResult.value == 3) {
            set.add('d');
        }
    }

    // test 'b' is deleted
    if (!set.has('b')) {
        print('b is deleted');
    }

    // test 'd' is added
    if (set.has('d')) {
        print('d is added');
    }
}

func1()
func2()
func3()
func4()
print(ArkTools.isAOTCompiled(func1))
print(ArkTools.isAOTCompiled(func2))
print(ArkTools.isAOTCompiled(func3))
print(ArkTools.isAOTCompiled(func4))

