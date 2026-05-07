/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

var binaryOperators = ["%", ">>", ">>>", "<<", "|"];

var interestingTop16bits = [0x8000, 0xffff,];

function makeFloat(top16) {
  var buf = new ArrayBuffer(8);
  var floats = new Float64Array(buf);
  var ints = new Uint16Array(buf);
  
  ints[0] = 0xacac;
  ints[1] = 0xdd33;
  ints[2] = 0x1b2f;
  ints[3] = top16;
  return floats[0];
}

function startFunction(n)
{
  print("function f" + n + "() {");
}

function endFunction()
{
  print("}");
}

function makeTestCase(op, val1, val2)
{
  val1 = makeFloat(val1);
  if (val2 == undefined) {
    print("print(" + op + " hide(" + val1 + "));");
  }
  else
  {
    val2 = makeFloat(val2);
    print("print(hide(" + val1 + ") " + op + " hide( " + val2 + "));");
  }
}

var fnc = 0;
for(var val1 in interestingTop16bits)
{
  startFunction(fnc);
  var val2, op;
  for (val2 in interestingTop16bits)
  {
    for (op in binaryOperators)
    {
      makeTestCase(binaryOperators[op], interestingTop16bits[val1], interestingTop16bits[val2]);
    }
  }
  endFunction();
  ++fnc;
}
