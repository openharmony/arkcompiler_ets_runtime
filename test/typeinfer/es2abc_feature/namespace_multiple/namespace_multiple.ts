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

declare function AssertType(value:any, type:string):void;

namespace A {
    export let a = 123;
    export let b = "abc";
}


namespace B {
    export let c = 456;
    export let d = "def";
}

AssertType(A.a + B.c, "int");
AssertType(A.a + A.b, "string");
AssertType(A.a + B.d, "string");
AssertType(A.b + B.c, "string");
AssertType(A.b + B.d, "string");
AssertType(B.c + B.d, "string");
