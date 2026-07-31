/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
declare function print(arg: any): string;

const REP = 60000;

function number_to_string_right(num: number): string {
    return "" + num;
}

function number_to_string_left(num: number): string {
    return num + "";
}

function number_to_string_mixed(num: number, prefix: string, suffix: string): string {
    return prefix + num + suffix;
}

for (let i = 0; i < REP; i++) {
    number_to_string_right(i);
    number_to_string_left(i);
    number_to_string_mixed(i, "value=", ";");
}

ArkTools.arkSteedCompileSync(number_to_string_right);
ArkTools.arkSteedCompileSync(number_to_string_left);
ArkTools.arkSteedCompileSync(number_to_string_mixed);

print(number_to_string_right(123));
print(number_to_string_right(-45.5));
print(number_to_string_left(100));
print(number_to_string_left(0));
print(number_to_string_mixed(7, "value=", ";"));
