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
{
    // test new builtin array
    let array1 = new Array();
    let array2 = new Array(1);

    print(array1.length);
    print(array2.length);

    // test call builtin constructor
    print(Array().length);   // c++ path
    print(Array(1).length);  // c++ path

    try {
        let array3 = new Array(-1);
        print(array3.length);
    } catch (e) {
        print("throw exception");
    }

    try {
        let array3 = new Array(2.1);
        print(array3.length);
    } catch (e) {
        print("throw exception");
    }

    try {
        let array3 = new Array(4294967296);
        print(array3.length);
    } catch (e) {
        print("throw exception");
    }
}