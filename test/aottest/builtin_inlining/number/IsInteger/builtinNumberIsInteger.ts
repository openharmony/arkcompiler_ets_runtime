/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

//aot: [trace] aot inline builtin: Number.isInteger, caller function name:func_main_0@builtinNumberIsInteger
print(Number.isInteger(true)); //: false
//aot: [trace] aot inline builtin: Number.isInteger, caller function name:func_main_0@builtinNumberIsInteger
print(Number.isInteger(false)); //: false

//aot: [trace] aot inline builtin: Number.isSafeInteger, caller function name:func_main_0@builtinNumberIsInteger
print(Number.isSafeInteger(true)); //: false
//aot: [trace] aot inline builtin: Number.isSafeInteger, caller function name:func_main_0@builtinNumberIsInteger
print(Number.isSafeInteger(false)); //: false
