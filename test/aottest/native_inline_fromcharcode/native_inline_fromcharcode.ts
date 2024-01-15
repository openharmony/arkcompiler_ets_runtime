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

declare function print(str:any):void;

print(String.fromCharCode(Number.NaN).charCodeAt(0))

print(String.fromCharCode(Number.POSITIVE_INFINITY).charCodeAt(0))

print(String.fromCharCode(68));

print(String.fromCharCode(131071).charCodeAt(0))

print(String.fromCharCode(4294967295).charCodeAt(0))

print(String.fromCharCode(4294967294).charCodeAt(0))

print(String.fromCharCode(4294967296).charCodeAt(0))

print(String.fromCharCode(-32767).charCodeAt(0))

print(String.fromCharCode(-65536).charCodeAt(0))

print(String.fromCharCode(-1.234).charCodeAt(0))

print(String.fromCharCode(-5.4321).charCodeAt(0))