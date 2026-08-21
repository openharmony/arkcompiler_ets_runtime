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
declare function print(arg: number): string;

function string_template_literal(name: string, age: number): string {
    let prefix = "Hello, ";
    let suffix = "! You are ";
    let years = " years old.";
    let result = prefix.concat(name).concat(suffix);
    let temp = age.toString();
    return result.concat(temp).concat(years);
}

ArkTools.arkSteedCompileSync(string_template_literal);


print(string_template_literal("Alice", 25));
print(string_template_literal("Bob", 30));
