/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/genericReversingTypeParameters.ts ===
declare function AssertType(value:any, type:string):void;
class BiMap<K, V> {
    private inverseBiMap: BiMap<V, K>;
    public get(key: K): V { 
AssertType(null, "null");
return null; 
}

    public inverse(): BiMap<V, K> { 
AssertType(null, "null");
return null; 
}
}

let b = new BiMap<string, number>();
AssertType(b, "BiMap<string, number>");
AssertType(new BiMap<string, number>(), "BiMap<string, number>");
AssertType(BiMap, "typeof BiMap");

let r1 = b.get(''); 
AssertType(r1, "number");
AssertType(b.get(''), "number");
AssertType(b.get, "(string) => number");
AssertType('', "string");

let i = b.inverse(); // used to get the type wrong here.
AssertType(i, "BiMap<number, string>");
AssertType(b.inverse(), "BiMap<number, string>");
AssertType(b.inverse, "() => BiMap<number, string>");

let r2b = i.get(1); 
AssertType(r2b, "string");
AssertType(i.get(1), "string");
AssertType(i.get, "(number) => string");
AssertType(1, "int");


