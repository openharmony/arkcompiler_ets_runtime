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

// @ts-nocheck
// rule-4 check

/*
 * Chain Rules:
 * 1: local_N(B) ∩ vtable_N(B) = ∅
 * 2: local(A) ⊆ local(B)
 * 3: vtable_NT(A) ⊆ vtable_NT(B)
 * 4: local_N(A) ∩ vtable_N(B) = ∅
 * 5: vtable_N(A) ∩ local_N(B) = ∅
 */

declare function print(arg:any):string;

class A {
    x:number;
}

class B extends A {
    x() {
        return "x";
    }
}

let a = new A();
print(a.x);


let b = new B();
print(b.x());
