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

// Test basic for of map.entries()
function func1() {
    let map = new Map([['a', 1], ['b', 2]]); 
    for (let ele of map) {
        print(ele);
    }
}

// Test basic for of map.keys()
function func2() {
    let map = new Map([['a', 1], ['b', 2]]); 
    for (let ele of map.keys()) {
        print(ele);
    }
}

// Test basic for of map.values()
function func3() {
    let map = new Map([['a', 1], ['b', 2]]); 
    for (let ele of map.values()) {
        print(ele);
    }
}

// Test update map 
function func4() {
    const map = new Map([[1, 'a'], [2, 'b'], [3, 'c']]);
  
    const iterator = Map.prototype[Symbol.iterator].call(map);
  
    for (let iterResult = iterator.next(); !iterResult.done; iterResult = iterator.next()) {
        print(iterResult.value);
        if (iterResult.value[1] == 'b') {
            map.set(3, 'd');
            map.delete(1);
            map.delete(2);
        }
    }

    for (let entry of map) {
        print(entry);
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
