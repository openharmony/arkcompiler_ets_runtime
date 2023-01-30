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

// === tests/cases/conformance/types/spread/objectSpreadRepeatedComplexity.ts ===
declare function AssertType(value:any, type:string):void;
function f(cnd: Record<number, boolean>){
    // Type is a union of 2^(n-1) members, where n is the number of spread objects
AssertType({        // Without this one, it collapses to {} ?        ...(cnd[1] &&            cnd[2] && {            prop0: 0,        }),        // With one prop each, it collapses to a single object (#34853?)        ...(cnd[3] && {            prop3a: 1,            prop3b: 1,        }),        ...(cnd[4] && {            prop4a: 1,            prop4b: 1,        }),        ...(cnd[5] && {            prop5a: 1,            prop5b: 1,        }),        ...(cnd[6] && {            prop6a: 1,            prop6b: 1,        }),        ...(cnd[7] && {            prop7a: 1,            prop7b: 1,        }),        ...(cnd[8] && {            prop8a: 1,            prop8b: 1,        }),        ...(cnd[9] && {            prop9a: 1,            prop9b: 1,        }),        ...(cnd[10] && {            prop10a: 1,            prop10b: 1,        }),        ...(cnd[11] && {            prop11a: 1,            prop11b: 1,        }),        ...(cnd[12] && {            prop12a: 1,            prop12b: 1,        }),        ...(cnd[13] && {            prop13a: 1,            prop13b: 1,        }),        ...(cnd[14] && {            prop14a: 1,            prop14b: 1,        }),        ...(cnd[15] && {            prop15a: 1,            prop15b: 1,        }),        ...(cnd[16] && {            prop16a: 1,            prop16b: 1,        }),        ...(cnd[17] && {            prop17a: 1,            prop17b: 1,        }),        ...(cnd[18] && {            prop18a: 1,            prop18b: 1,        }),        ...(cnd[19] && {            prop19a: 1,            prop19b: 1,        }),        ...(cnd[20] && {            prop20a: 1,            prop20b: 1,        }),    }, "{ prop20a?: union; prop20b?: union; prop19a?: union; prop19b?: union; prop18a?: union; prop18b?: union; prop17a?: union; prop17b?: union; prop16a?: union; prop16b?: union; prop15a?: union; prop15b?: union; prop14a?: union; prop14b?: union; prop13a?: union; prop13b?: union; prop12a?: union; prop12b?: union; prop11a?: union; prop11b?: union; prop10a?: union; prop10b?: union; prop9a?: union; prop9b?: union; prop8a?: union; prop8b?: union; prop7a?: union; prop7b?: union; prop6a?: union; prop6b?: union; prop5a?: union; prop5b?: union; prop4a?: union; prop4b?: union; prop3a?: union; prop3b?: union; prop0?: union; }");
    return {

        // Without this one, it collapses to {} ?
        ...(cnd[1] &&
AssertType((cnd[1] &&            cnd[2] && {            prop0: 0,        }), "union");
AssertType(cnd[1] &&            cnd[2] && {            prop0: 0,        }, "union");
AssertType(cnd[1] &&            cnd[2], "boolean");
AssertType(cnd[1], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(1, "int");

            cnd[2] && {
AssertType(cnd[2], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(2, "int");
AssertType({            prop0: 0,        }, "{ prop0: number; }");

            prop0: 0,
AssertType(prop0, "number");
AssertType(0, "int");

        }),

        // With one prop each, it collapses to a single object (#34853?)
        ...(cnd[3] && {
AssertType((cnd[3] && {            prop3a: 1,            prop3b: 1,        }), "union");
AssertType(cnd[3] && {            prop3a: 1,            prop3b: 1,        }, "union");
AssertType(cnd[3], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(3, "int");
AssertType({            prop3a: 1,            prop3b: 1,        }, "{ prop3a: number; prop3b: number; }");

            prop3a: 1,
AssertType(prop3a, "number");
AssertType(1, "int");

            prop3b: 1,
AssertType(prop3b, "number");
AssertType(1, "int");

        }),
        ...(cnd[4] && {
AssertType((cnd[4] && {            prop4a: 1,            prop4b: 1,        }), "union");
AssertType(cnd[4] && {            prop4a: 1,            prop4b: 1,        }, "union");
AssertType(cnd[4], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(4, "int");
AssertType({            prop4a: 1,            prop4b: 1,        }, "{ prop4a: number; prop4b: number; }");

            prop4a: 1,
AssertType(prop4a, "number");
AssertType(1, "int");

            prop4b: 1,
AssertType(prop4b, "number");
AssertType(1, "int");

        }),
        ...(cnd[5] && {
AssertType((cnd[5] && {            prop5a: 1,            prop5b: 1,        }), "union");
AssertType(cnd[5] && {            prop5a: 1,            prop5b: 1,        }, "union");
AssertType(cnd[5], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(5, "int");
AssertType({            prop5a: 1,            prop5b: 1,        }, "{ prop5a: number; prop5b: number; }");

            prop5a: 1,
AssertType(prop5a, "number");
AssertType(1, "int");

            prop5b: 1,
AssertType(prop5b, "number");
AssertType(1, "int");

        }),
        ...(cnd[6] && {
AssertType((cnd[6] && {            prop6a: 1,            prop6b: 1,        }), "union");
AssertType(cnd[6] && {            prop6a: 1,            prop6b: 1,        }, "union");
AssertType(cnd[6], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(6, "int");
AssertType({            prop6a: 1,            prop6b: 1,        }, "{ prop6a: number; prop6b: number; }");

            prop6a: 1,
AssertType(prop6a, "number");
AssertType(1, "int");

            prop6b: 1,
AssertType(prop6b, "number");
AssertType(1, "int");

        }),
        ...(cnd[7] && {
AssertType((cnd[7] && {            prop7a: 1,            prop7b: 1,        }), "union");
AssertType(cnd[7] && {            prop7a: 1,            prop7b: 1,        }, "union");
AssertType(cnd[7], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(7, "int");
AssertType({            prop7a: 1,            prop7b: 1,        }, "{ prop7a: number; prop7b: number; }");

            prop7a: 1,
AssertType(prop7a, "number");
AssertType(1, "int");

            prop7b: 1,
AssertType(prop7b, "number");
AssertType(1, "int");

        }),
        ...(cnd[8] && {
AssertType((cnd[8] && {            prop8a: 1,            prop8b: 1,        }), "union");
AssertType(cnd[8] && {            prop8a: 1,            prop8b: 1,        }, "union");
AssertType(cnd[8], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(8, "int");
AssertType({            prop8a: 1,            prop8b: 1,        }, "{ prop8a: number; prop8b: number; }");

            prop8a: 1,
AssertType(prop8a, "number");
AssertType(1, "int");

            prop8b: 1,
AssertType(prop8b, "number");
AssertType(1, "int");

        }),
        ...(cnd[9] && {
AssertType((cnd[9] && {            prop9a: 1,            prop9b: 1,        }), "union");
AssertType(cnd[9] && {            prop9a: 1,            prop9b: 1,        }, "union");
AssertType(cnd[9], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(9, "int");
AssertType({            prop9a: 1,            prop9b: 1,        }, "{ prop9a: number; prop9b: number; }");

            prop9a: 1,
AssertType(prop9a, "number");
AssertType(1, "int");

            prop9b: 1,
AssertType(prop9b, "number");
AssertType(1, "int");

        }),
        ...(cnd[10] && {
AssertType((cnd[10] && {            prop10a: 1,            prop10b: 1,        }), "union");
AssertType(cnd[10] && {            prop10a: 1,            prop10b: 1,        }, "union");
AssertType(cnd[10], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(10, "int");
AssertType({            prop10a: 1,            prop10b: 1,        }, "{ prop10a: number; prop10b: number; }");

            prop10a: 1,
AssertType(prop10a, "number");
AssertType(1, "int");

            prop10b: 1,
AssertType(prop10b, "number");
AssertType(1, "int");

        }),
        ...(cnd[11] && {
AssertType((cnd[11] && {            prop11a: 1,            prop11b: 1,        }), "union");
AssertType(cnd[11] && {            prop11a: 1,            prop11b: 1,        }, "union");
AssertType(cnd[11], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(11, "int");
AssertType({            prop11a: 1,            prop11b: 1,        }, "{ prop11a: number; prop11b: number; }");

            prop11a: 1,
AssertType(prop11a, "number");
AssertType(1, "int");

            prop11b: 1,
AssertType(prop11b, "number");
AssertType(1, "int");

        }),
        ...(cnd[12] && {
AssertType((cnd[12] && {            prop12a: 1,            prop12b: 1,        }), "union");
AssertType(cnd[12] && {            prop12a: 1,            prop12b: 1,        }, "union");
AssertType(cnd[12], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(12, "int");
AssertType({            prop12a: 1,            prop12b: 1,        }, "{ prop12a: number; prop12b: number; }");

            prop12a: 1,
AssertType(prop12a, "number");
AssertType(1, "int");

            prop12b: 1,
AssertType(prop12b, "number");
AssertType(1, "int");

        }),
        ...(cnd[13] && {
AssertType((cnd[13] && {            prop13a: 1,            prop13b: 1,        }), "union");
AssertType(cnd[13] && {            prop13a: 1,            prop13b: 1,        }, "union");
AssertType(cnd[13], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(13, "int");
AssertType({            prop13a: 1,            prop13b: 1,        }, "{ prop13a: number; prop13b: number; }");

            prop13a: 1,
AssertType(prop13a, "number");
AssertType(1, "int");

            prop13b: 1,
AssertType(prop13b, "number");
AssertType(1, "int");

        }),
        ...(cnd[14] && {
AssertType((cnd[14] && {            prop14a: 1,            prop14b: 1,        }), "union");
AssertType(cnd[14] && {            prop14a: 1,            prop14b: 1,        }, "union");
AssertType(cnd[14], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(14, "int");
AssertType({            prop14a: 1,            prop14b: 1,        }, "{ prop14a: number; prop14b: number; }");

            prop14a: 1,
AssertType(prop14a, "number");
AssertType(1, "int");

            prop14b: 1,
AssertType(prop14b, "number");
AssertType(1, "int");

        }),
        ...(cnd[15] && {
AssertType((cnd[15] && {            prop15a: 1,            prop15b: 1,        }), "union");
AssertType(cnd[15] && {            prop15a: 1,            prop15b: 1,        }, "union");
AssertType(cnd[15], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(15, "int");
AssertType({            prop15a: 1,            prop15b: 1,        }, "{ prop15a: number; prop15b: number; }");

            prop15a: 1,
AssertType(prop15a, "number");
AssertType(1, "int");

            prop15b: 1,
AssertType(prop15b, "number");
AssertType(1, "int");

        }),
        ...(cnd[16] && {
AssertType((cnd[16] && {            prop16a: 1,            prop16b: 1,        }), "union");
AssertType(cnd[16] && {            prop16a: 1,            prop16b: 1,        }, "union");
AssertType(cnd[16], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(16, "int");
AssertType({            prop16a: 1,            prop16b: 1,        }, "{ prop16a: number; prop16b: number; }");

            prop16a: 1,
AssertType(prop16a, "number");
AssertType(1, "int");

            prop16b: 1,
AssertType(prop16b, "number");
AssertType(1, "int");

        }),
        ...(cnd[17] && {
AssertType((cnd[17] && {            prop17a: 1,            prop17b: 1,        }), "union");
AssertType(cnd[17] && {            prop17a: 1,            prop17b: 1,        }, "union");
AssertType(cnd[17], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(17, "int");
AssertType({            prop17a: 1,            prop17b: 1,        }, "{ prop17a: number; prop17b: number; }");

            prop17a: 1,
AssertType(prop17a, "number");
AssertType(1, "int");

            prop17b: 1,
AssertType(prop17b, "number");
AssertType(1, "int");

        }),
        ...(cnd[18] && {
AssertType((cnd[18] && {            prop18a: 1,            prop18b: 1,        }), "union");
AssertType(cnd[18] && {            prop18a: 1,            prop18b: 1,        }, "union");
AssertType(cnd[18], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(18, "int");
AssertType({            prop18a: 1,            prop18b: 1,        }, "{ prop18a: number; prop18b: number; }");

            prop18a: 1,
AssertType(prop18a, "number");
AssertType(1, "int");

            prop18b: 1,
AssertType(prop18b, "number");
AssertType(1, "int");

        }),
        ...(cnd[19] && {
AssertType((cnd[19] && {            prop19a: 1,            prop19b: 1,        }), "union");
AssertType(cnd[19] && {            prop19a: 1,            prop19b: 1,        }, "union");
AssertType(cnd[19], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(19, "int");
AssertType({            prop19a: 1,            prop19b: 1,        }, "{ prop19a: number; prop19b: number; }");

            prop19a: 1,
AssertType(prop19a, "number");
AssertType(1, "int");

            prop19b: 1,
AssertType(prop19b, "number");
AssertType(1, "int");

        }),
        ...(cnd[20] && {
AssertType((cnd[20] && {            prop20a: 1,            prop20b: 1,        }), "union");
AssertType(cnd[20] && {            prop20a: 1,            prop20b: 1,        }, "union");
AssertType(cnd[20], "boolean");
AssertType(cnd, "Record<number, boolean>");
AssertType(20, "int");
AssertType({            prop20a: 1,            prop20b: 1,        }, "{ prop20a: number; prop20b: number; }");

            prop20a: 1,
AssertType(prop20a, "number");
AssertType(1, "int");

            prop20b: 1,
AssertType(prop20b, "number");
AssertType(1, "int");

        }),
    };
}

