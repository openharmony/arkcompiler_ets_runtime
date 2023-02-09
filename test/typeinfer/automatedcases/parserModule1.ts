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

// === tests/cases/conformance/parser/ecmascript5/ModuleDeclarations/parserModule1.ts ===
declare function AssertType(value:any, type:string):void;
    export module CompilerDiagnostics {
        export let debug = false;
        export interface IDiagnosticWriter {
            Alert(output: string): void;
        }

        export let diagnosticWriter: IDiagnosticWriter = null;

        export let analysisPass: number = 0;

        export function Alert(output: string) {
            if (diagnosticWriter) {
AssertType(diagnosticWriter, "IDiagnosticWriter");

                diagnosticWriter.Alert(output);
AssertType(diagnosticWriter.Alert(output), "void");
AssertType(diagnosticWriter.Alert, "(string) => void");
AssertType(output, "string");
            }
        }

        export function debugPrint(s: string) {
            if (debug) {
AssertType(debug, "boolean");

                Alert(s);
AssertType(Alert(s), "void");
AssertType(Alert, "(string) => void");
AssertType(s, "string");
            }
        }

        export function assert(condition: boolean, s: string) {
            if (debug) {
AssertType(debug, "boolean");

                if (!condition) {
AssertType(!condition, "boolean");
AssertType(condition, "boolean");

                    Alert(s);
AssertType(Alert(s), "void");
AssertType(Alert, "(string) => void");
AssertType(s, "string");
                }
            }
        }

    }

