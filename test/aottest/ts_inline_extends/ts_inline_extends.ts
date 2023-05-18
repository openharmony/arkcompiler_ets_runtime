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

declare function print(arg:any):string;

class A {
    run() {
      print("A.run")
    }
}
  
class B extends A {
    run() {
        print("B.run")
    }
}
  
class C	{
    obj: A
    constructor(obj: A) {
        this.obj = obj
    }
    
    foo() {
        this.obj.run()
    }
}
let b = new B()
let c = new C(b)
c.foo();