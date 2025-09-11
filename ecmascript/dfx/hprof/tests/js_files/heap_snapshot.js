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

let str1 = 'string1';
let str2 = 'string2';
let str3 = 'string3';
let str4 = 'string4';
let str5 = 'string5';
let str6 = 'string6';
let str7 = 'string7';
let str8 = 'string8';

globalThis.treeStr1 = str1 + str2;
globalThis.treeStr2 = str3 + str4;
globalThis.treeStr3 = str5 + str6;
globalThis.treeStr4 = str7 + str8;
globalThis.treeStr5 = str1 + str2 + str3 + str4;
globalThis.treeStr6 = str5 + str6 + str7 + str8;
globalThis.treeStr7 = str1 + str2 + str3 + str4 + str5 + str6 + str7 + str8;

globalThis.save1 = str1;
globalThis.save2 = str2;
globalThis.save3 = str3;
globalThis.save4 = str4;
globalThis.save5 = str5;
globalThis.save6 = str6;
globalThis.save7 = str7;
globalThis.save8 = str8;