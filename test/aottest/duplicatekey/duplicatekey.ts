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
declare var ArkTools:any;
{
    var hca;
    var hcb;
    class A {
        value:number;
        constructor (value:number) {
            hca = ArkTools.getHClass(this);
            this.value = value;
        }
    }

    class B extends A {
        value:number;
        name:string;
        constructor (value:number, name:string) {
            super(value);
            this.name = name;
            hcb = ArkTools.getHClass(this);
        }
    }

    let a = new A(123);
    let hc1 = ArkTools.getHClass(a);
    print(hca === hc1);
    print(ArkTools.isTSHClass(a));

    let b = new B(456, "abc");
    let hc2 = ArkTools.getHClass(b);
    print(hcb === hc2);
    print(ArkTools.isTSHClass(b));

    print(a.value);
    print(b.value);
    print(b.name);
}
