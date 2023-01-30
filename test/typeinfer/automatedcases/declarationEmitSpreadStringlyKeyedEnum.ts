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

// === tests/cases/compiler/declarationEmitSpreadStringlyKeyedEnum.ts ===
declare function AssertType(value:any, type:string):void;
enum AgeGroups { "0-17" , "18-22" , "23-27" , "28-34" , "35-44" , "45-59" , "60-150" }
export const SpotifyAgeGroupEnum = { ...AgeGroups };
AssertType(SpotifyAgeGroupEnum, "{ [number]: string; "0-17": (typeof AgeGroups)["0-17"]; "18-22": (typeof AgeGroups)["18-22"]; "23-27": (typeof AgeGroups)["23-27"]; "28-34": (typeof AgeGroups)["28-34"]; "35-44": (typeof AgeGroups)["35-44"]; "45-59": (typeof AgeGroups)["45-59"]; "60-150": (typeof AgeGroups)["60-150"]; }");
AssertType({ ...AgeGroups }, "{ [number]: string; "0-17": (typeof AgeGroups)["0-17"]; "18-22": (typeof AgeGroups)["18-22"]; "23-27": (typeof AgeGroups)["23-27"]; "28-34": (typeof AgeGroups)["28-34"]; "35-44": (typeof AgeGroups)["35-44"]; "45-59": (typeof AgeGroups)["45-59"]; "60-150": (typeof AgeGroups)["60-150"]; }");
AssertType(AgeGroups, "typeof AgeGroups");


