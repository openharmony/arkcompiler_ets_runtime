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
{
    // test new builtin array
    let numObj1 = "-100";
    print(Number.parseInt(numObj1));

    let numObj2 = "1000";
    print(Number.parseInt(numObj2));

    let numObj3 = "0001030";
    print(Number.parseInt(numObj3));

    let numObj4 = "   -1123";
    print(Number.parseInt(numObj4));
}
