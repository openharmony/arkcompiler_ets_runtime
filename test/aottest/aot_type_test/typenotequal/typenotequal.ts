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

declare function print(arg:any):string;
let a:number = 1;
let b:number = 2;
print(a != b);

let str1:string = "a"
let str2:string = "b"
print(str1 != str2);


let c:undefined = undefined;
let d:null = null;
print(c != d);

let bool1:boolean = true;
let bool2:boolean = true;
print(bool1 != bool2);
