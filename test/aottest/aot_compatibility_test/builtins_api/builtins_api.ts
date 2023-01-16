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

declare function print(arg:string, arg2:any):string;

{
    class D {
        b:string;
        constructor() {
            print("this.hasOwnProperty:", this.hasOwnProperty("b"));
            print("Object.getOwnPropertyNames:", Object.getOwnPropertyNames(this));
            print("Object.getOwnPropertyDescriptor:", Object.getOwnPropertyDescriptor(this,"b"));
            print("Object.keys():", Object.keys(this));
            print("Reflect.has:", Reflect.has(this, "b"));
            print("Reflect.ownKeys():", Reflect.ownKeys(this));
            print("Reflect.get:", Reflect.get(this, "b"));
            print("Reflect.getOwnPropertyDescriptor():", Reflect.getOwnPropertyDescriptor(this, "b"));
            this.b = "abc";
            print("this.hasOwnProperty:", this.hasOwnProperty("b"));
            print("Object.getOwnPropertyNames:", Object.getOwnPropertyNames(this));
            print("Object.getOwnPropertyDescriptor:", Object.getOwnPropertyDescriptor(this,"b"));
            print("Object.keys():", Object.keys(this));
            print("Reflect.has:", Reflect.has(this, "b"));
            print("Reflect.ownKeys():", Reflect.ownKeys(this));
            print("Reflect.get:", Reflect.get(this, "b"));
            print("Reflect.getOwnPropertyDescriptor():", Reflect.getOwnPropertyDescriptor(this, "b"));
        }
    }

    let d = new D();
}