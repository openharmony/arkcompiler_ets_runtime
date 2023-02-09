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

// === tests/cases/compiler/declFileConstructSignatures_1.ts ===
declare function AssertType(value:any, type:string):void;

interface IGlobalConstructSignature {
    /** This comment should appear for foo*/
    new (): string;
}

interface IGlobalConstructSignatureWithParameters {
    /** This is comment for function signature*/
    new (/** this is comment about a*/a: string,
        /** this is comment for b*/
        b: number);
}

interface IGlobalConstructSignatureWithRestParameters {

    new (a: string, ...rests: string[]): string;

}

interface IGlobalConstructSignatureWithOverloads {
    new (a: string): string;
    new (a: number): number;
}

interface IGlobalConstructSignatureWithTypeParameters<T> {
    /** This comment should appear for foo*/
    new (a: T): T;
}

interface IGlobalConstructSignatureWithOwnTypeParametes {
    new <T extends IGlobalConstructSignature>(a: T): T;
}

