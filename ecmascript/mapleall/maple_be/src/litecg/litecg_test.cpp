/*
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
 */

/*
  a basic example to use litecg & lmir builder API.

  At this stage, it shows:
  - The basic workflow of using litecg API.
  - using lmir builder API to construct in-memory IR input to litecg.
  - and then dump the input IR to a text-format maple IR file.
 */

#include "litecg.h"

using namespace maple::litecg;

#define __ irBuilder->  // make the code looks better

void generateIR(LMIRBuilder *irBuilder)
{
    /* case 1: Note here parameters are implicitly defined without return
               Var handle, thus requires GetLocalVar.

      i32 function1(i32 param1, i64 param2) {
        return param1 + (i32) param2;
      }
     */
    Function &function1 = __ DefineFunction("function1")
                              .Param(__ i32Type, "param1")
                              .Param(__ i64Type, "param2")
                              .Return(__ i32Type)
                              .Done();

    __ SetCurFunc(function1);

    BB &bb = __ CreateBB();
    Stmt &retStmt = __ Return(__ Add(__ i32Type, __ Dread(__ GetLocalVar("param1")),
                                     __ Trunc(__ i64Type, __ i32Type, __ Dread(__ GetLocalVar("param2")))));
    __ AppendStmt(bb, retStmt);
    __ AppendBB(bb);

    // TODO: to be complete
    /* case 2

     */
}

int main()
{
    LiteCG liteCG("lmirexample");
    auto irBuilder = liteCG.GetIRBuilder();
    generateIR(&irBuilder);

    liteCG.DumpIRToFile("lmirexample.mpl");
    return 0;
}
