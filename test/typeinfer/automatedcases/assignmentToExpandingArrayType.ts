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

// === tests/cases/compiler/assignmentToExpandingArrayType.ts ===
declare function AssertType(value:any, type:string):void;
// Fixes exponential time/space in #14628
let x = []
AssertType(x, "any[]");
AssertType([], "undefined[]");

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' } // previously ran out of memory here
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");
AssertType(x[0], "any");
AssertType(x, "any[]");
AssertType(0, "int");
AssertType({ foo: 'hi' }, "{ foo: string; }");
AssertType(foo, "string");
AssertType('hi', "string");

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}

x[0] = { foo: 'hi' 
AssertType(x[0] = { foo: 'hi' }, "{ foo: string; }");

AssertType(x[0], "any");

AssertType(x, "any[]");

AssertType(0, "int");

AssertType({ foo: 'hi' }, "{ foo: string; }");

AssertType(foo, "string");

AssertType('hi', "string");
}


