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

// === tests/cases/compiler/declarationEmitBindingPatternWithReservedWord.ts ===
declare function AssertType(value:any, type:string):void;
type LocaleData = Record<string, never>
type ConvertLocaleConfig<T extends LocaleData = LocaleData> = Record<
  string,
  T
>;
type LocaleConfig<T extends LocaleData = LocaleData> = Record<string, Partial<T>>;

export interface GetLocalesOptions<T extends LocaleData> {
    app: unknown;
    default: ConvertLocaleConfig<T>;
    config?: LocaleConfig<T> | undefined;
    name?: string;
}

export const getLocales = <T extends LocaleData>({
AssertType(getLocales, "<T extends LocaleData>(GetLocalesOptions<T>) => ConvertLocaleConfig<T>");
AssertType(<T extends LocaleData>({    app,    name,    default: defaultLocalesConfig,    config: userLocalesConfig = {},}: GetLocalesOptions<T>): ConvertLocaleConfig<T> => {    return defaultLocalesConfig;}, "<T extends LocaleData>(GetLocalesOptions<T>) => ConvertLocaleConfig<T>");

    app,
AssertType(app, "unknown");

    name,
AssertType(name, "string");

    default: defaultLocalesConfig,
AssertType(default, "any");
AssertType(defaultLocalesConfig, "ConvertLocaleConfig<T>");

    config: userLocalesConfig = {},
AssertType(config, "any");
AssertType(userLocalesConfig, "LocaleConfig<T>");
AssertType({}, "{}");

}: GetLocalesOptions<T>): ConvertLocaleConfig<T> => {
AssertType(defaultLocalesConfig, "ConvertLocaleConfig<T>");
    return defaultLocalesConfig;

};


