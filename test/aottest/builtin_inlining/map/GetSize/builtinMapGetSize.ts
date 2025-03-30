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

declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}
declare function print(arg:any):string;

function doSize(): number {
    return myMap.size;
}

function printSize() {
    try {
        print(doSize());
    } finally {
    }
}

let myMap = new Map([[0, 0], [0.0, 5], [-1, 1], [2.5, -2.5], [NaN, Infinity], [2000, -0.0], [56, "oops"], ["xyz", "12345"]]);

// Check without params
print(myMap.size); //: 7
printSize(); //: 7

// Check IR correctness inside try-block
try {
    print(myMap.size); //: 7
    printSize(); //: 7
} catch (e) {
}

// Check after deleting elements
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapGetSize
myMap.delete(-1);
print(myMap.size); //: 6
//aot: [trace] aot inline builtin: Map.delete, caller function name:func_main_0@builtinMapGetSize
myMap.delete(-200);
print(myMap.size); //: 6

// Check after inserting elements
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapGetSize
myMap.set(2000, 1e-98);
print(myMap.size); //: 6
//aot: [trace] aot call builtin: Map.Set, caller function name:func_main_0@builtinMapGetSize
myMap.set(133.33, -1);
print(myMap.size); //: 7

// Check after clearing
myMap.clear();
//aot: [trace] aot call builtin: Map.clear, caller function name:func_main_0@builtinMapGetSize
print(myMap.size); //: 0

// Check deoptimization
if (ArkTools.isAOTCompiled(printSize)) {
    // Define 'size' property in 'myMap', which shadows 'prototype.size'
    Object.defineProperty(myMap, 'size', {
        value: 42
    });
}

printSize();
//pgo: 0
//aot: [trace] Check Type: IsNotMap
//aot: 42
