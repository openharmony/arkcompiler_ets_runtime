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


export class foo1{
    b1 = (para1 : string, para2 : string) : string => {return para1 + para2};
    b2 = 1;
}

export class foo2 {
    a1 = 1;
    a2 = "abc";
    a3 = new foo1();
    a4 = (para1 : number, para2 : number) : number => {return para1 + para2};
}
