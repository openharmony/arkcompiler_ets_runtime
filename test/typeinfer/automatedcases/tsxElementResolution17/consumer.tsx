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

// === tests/cases/conformance/jsx/consumer.tsx ===
declare function AssertType(value:any, type:string):void;
///<reference path="file.tsx" />
// Should keep s1 and elide s2
import * as s1 from 'elements1';
import * as s2 from 'elements2';
<s1.MyElement />;
AssertType(<s1.MyElement />, "JSX.Element");
AssertType(s1.MyElement, "typeof s1.MyElement");


