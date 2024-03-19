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
declare function assert_unreachable():void;
declare function assert_equal(a: Object, b: Object):void;

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
      assert_unreachable();
    } catch(e) {
    }
}

// case4: GetDeps
function f2() {
    try {
        return
        assert_unreachable();
    } catch(e15) {
    }
    while(1) {}
}

// case5: asan check
function f59() : number[] {}
class C60 extends f59 {}
const v75 : number = new C60()

// case6: NewJSError Stack Overflow
function f230():string {
    const v292:number = -2;
    const v303:number[] = [1000,-1970969808,536870887,257,17933,11,1024];
    try { new v292(64, ...v303, ...v303, ...v303, ...v303); assert_unreachable(); } catch (e) {}
    return "bigint";
}
for (let v316:number = 0; v316 < 5; v316++) {
    f230();
}

// case7: extendClassType check
const v1455:{} = ("boolean")[Symbol.iterator];
try {
    class C1723 extends v1455 {
        [a1718];
        static [v1705] = a1719;
    }
    assert_unreachable();
} catch (e) {}

// case8: stackmapNum % 2
let v39:any = 129 || C3;
Math.sign("12");
v39++;

// case9: this branch is unreachable
let v:number = [];
assert_equal(Math.abs(--v), 1);
assert_equal(v, -1);
let u:number[] = [];
assert_equal(--u, -1);

print("compiler success");