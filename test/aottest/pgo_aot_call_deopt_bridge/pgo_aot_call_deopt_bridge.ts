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

let isAot = false;

function foo(a, b, c, d, e, f, g) {
    if (isAot) {
        ArkTools.forceFullGC()
    }
    if (isAot && g != undefined) {
        print(arguments.length)
        print(a)
        print(b)
        print(c)
        print(d)
        print(e)
        print(f)
        print(g)
    }
    return a + b
}

foo(1, 2)
print(ArkTools.isAOTCompiled(foo))  // pgo:false, aot:true
isAot = ArkTools.isAOTCompiled(foo)

// reach the deopt threshold of foo
if (isAot) {
    for (let i = 0; i < 20; i++) {
        ArkTools.forceFullGC()
        foo("a", 1)
    }
}
print(ArkTools.isAOTCompiled(foo))  // pgo:false, aot:false

function bar () {
    let testGC = {x: "gc"}
    if (isAot) {
        // slow path
        print(foo())            // aot:NaN
        print(foo(1))           // aot:NaN
        print(foo(3, 4))        // 7
    }
    // bridge
    print(foo(3, 4))            // 7
    print(foo(3, 4, 5))         // 7
    print(foo(3, 4, 5, 6))      // 7
    print(foo(3, 4, 5, 6, 7))   // 7
    print(foo(3, 4, 5, 6, 7, 8))   // 7
    print("long params1:")
    print(foo(13, 14, 15, 16, 17, 18, 19))   // 27
    print("long params2:")
    print(foo(23, 24, 25, 26, 27, 28, 29, 30))   // 47
    print(testGC.x)             // gc
}

bar()
print(ArkTools.isAOTCompiled(bar))  // pgo:false, aot:true