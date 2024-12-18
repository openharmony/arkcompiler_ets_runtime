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

function ReturnObjValue(obj)
{
    return obj.x;
}

var obj1 = {x: 1};

var y = 3;
var x = "test string";
var obj2 = {y, x};

var sum = 0;
for (var i = 0; i < 10000000; i++) {
    sum += ReturnObjValue(obj1);
}

print(sum);

print(ReturnObjValue(obj2));
