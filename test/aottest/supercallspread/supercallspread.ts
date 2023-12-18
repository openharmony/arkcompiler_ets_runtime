/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

class A {
    name:string;
    value:number;
    constructor(name:string, value:number) {
        this.name = name;
        this.value = value;
    }
}
class B extends A{
    constructor(...b: ConstructorParameters<typeof A>) {
        super(...b);
    }
}

var obj = new B("AOT", 123);
print(obj.name);
print(obj.value);

class Demo { 
    constructor(x, y) {
      this.x = x;
      this.y = y;
    }
    aboutToDo(){
      this.x = 0;
      this.y = 0;
    }
}
    
class Demo2 extends Demo{
aboutToDo(){
    super.aboutToDo();
}
}

let d = new Demo2('hello', 'bye');
print(d.x);
d.aboutToDo();
print(d.x);
