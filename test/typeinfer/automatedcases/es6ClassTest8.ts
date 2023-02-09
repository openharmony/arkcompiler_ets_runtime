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

// === tests/cases/compiler/es6ClassTest8.ts ===
declare function AssertType(value:any, type:string):void;
function f1(x:any) {
AssertType(x, "any");
return x;
}

class C {
    constructor() {
        let bar:any = (function() {
AssertType(bar, "any");
AssertType(function() {            return bar; // 'bar' should be resolvable        }, "() => any");
AssertType((function() {            return bar; // 'bar' should be resolvable        }), "() => any");

AssertType(bar, "any");
            return bar; // 'bar' should be resolvable

        });
        let b = f1(f1(bar));
AssertType(b, "any");
AssertType(f1(f1(bar)), "any");
AssertType(f1, "(any) => any");
AssertType(f1(bar), "any");
AssertType(f1, "(any) => any");
AssertType(bar, "any");
    }
  
}

class Vector {
    static norm(v:Vector):Vector {
AssertType(null, "null");
return null;
}

    static minus(v1:Vector, v2:Vector):Vector {
AssertType(null, "null");
return null;
}

    static times(v1:Vector, v2:Vector):Vector {
AssertType(null, "null");
return null;
}

    static cross(v1:Vector, v2:Vector):Vector {
AssertType(null, "null");
return null;
}

    constructor(public x: number,
                public y: number,
                public z: number) { 
    }

    static dot(v1:Vector, v2:Vector):Vector {
AssertType(null, "null");
return null;}    

}              

class Camera {
    public forward: Vector;
    public right: Vector;
    public up: Vector;
    constructor(public pos: Vector, lookAt: Vector) { 
        let down = new Vector(0.0, -1.0, 0.0);
AssertType(down, "Vector");
AssertType(new Vector(0.0, -1.0, 0.0), "Vector");
AssertType(Vector, "typeof Vector");
AssertType(0.0, "int");
AssertType(-1.0, "int");
AssertType(1.0, "int");
AssertType(0.0, "int");

        this.forward = Vector.norm(Vector.minus(lookAt,this.pos));
AssertType(this.forward = Vector.norm(Vector.minus(lookAt,this.pos)), "Vector");
AssertType(this.forward, "Vector");
AssertType(this, "this");
AssertType(Vector.norm(Vector.minus(lookAt,this.pos)), "Vector");
AssertType(Vector.norm, "(Vector) => Vector");
AssertType(Vector.minus(lookAt,this.pos), "Vector");
AssertType(Vector.minus, "(Vector, Vector) => Vector");
AssertType(lookAt, "Vector");
AssertType(this.pos, "Vector");
AssertType(this, "this");

        this.right = Vector.times(down, Vector.norm(Vector.cross(this.forward, down)));
AssertType(this.right = Vector.times(down, Vector.norm(Vector.cross(this.forward, down))), "Vector");
AssertType(this.right, "Vector");
AssertType(this, "this");
AssertType(Vector.times(down, Vector.norm(Vector.cross(this.forward, down))), "Vector");
AssertType(Vector.times, "(Vector, Vector) => Vector");
AssertType(down, "Vector");
AssertType(Vector.norm(Vector.cross(this.forward, down)), "Vector");
AssertType(Vector.norm, "(Vector) => Vector");
AssertType(Vector.cross(this.forward, down), "Vector");
AssertType(Vector.cross, "(Vector, Vector) => Vector");
AssertType(this.forward, "Vector");
AssertType(this, "this");
AssertType(down, "Vector");

        this.up = Vector.times(down, Vector.norm(Vector.cross(this.forward, this.right)));    
AssertType(this.up = Vector.times(down, Vector.norm(Vector.cross(this.forward, this.right))), "Vector");
AssertType(this.up, "Vector");
AssertType(this, "this");
AssertType(Vector.times(down, Vector.norm(Vector.cross(this.forward, this.right))), "Vector");
AssertType(Vector.times, "(Vector, Vector) => Vector");
AssertType(down, "Vector");
AssertType(Vector.norm(Vector.cross(this.forward, this.right)), "Vector");
AssertType(Vector.norm, "(Vector) => Vector");
AssertType(Vector.cross(this.forward, this.right), "Vector");
AssertType(Vector.cross, "(Vector, Vector) => Vector");
AssertType(this.forward, "Vector");
AssertType(this, "this");
AssertType(this.right, "Vector");
AssertType(this, "this");
    }
}



