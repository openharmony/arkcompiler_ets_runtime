/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
// import { BenchmarkRunner } from "../../../utils/benchmarkTsSuite";
// declare function print(arg:string):string;

declare interface ArkTools{
  timeInUs(arg:any):number
}

class Px {
  V: number[] = new Array();
  constructor(X: number,Y: number,Z: number){
    this.V = [X,Y,Z,1];
  }
}

class QType {
  arr: Px[] = new Array(9);
  LastPx: number = 0;
  Lastx:number = 0;
  Lasty:number = 0;
  Normal: Float64Array[] = new Array();
  Line: boolean[] = new Array();
  Edge: number[][] = new Array();
  NumPx: number = 0;
  constructor(){}
}

function run() {
  let Q: QType = new QType();
  let MTrans: number[][] = new Array();

  let MQube: number[][] = new Array();

  let I: number[][] = new Array();

  let Origin = new Array<number>();
  class  Testing {
    static LoopCount:number= 0
    static LoopMax:number= 0
  }

  let validation = new Map<number,number>([
    [20,2889.0000000000045],
    [40,2889.0000000000055],
    [80,2889.000000000005],
    [160,2889.0000000000055]
  ])
  // let validation = {
  //   20: 2889.0000000000045,
  //   40: 2889.0000000000055,
  //   80: 2889.000000000005,
  //   160: 2889.0000000000055
  // };

  let DrawLine:(From: Px, To: Px)=>void=(From: Px, To: Px):void=> {
    let x1 = From.V[0];
    let x2 = To.V[0];
    let y1 = From.V[1];
    let y2 = To.V[1];
    let dx = Math.abs(x2 - x1);
    let dy = Math.abs(y2 - y1);
    let x = x1;
    let y = y1;
    let IncX1: number, IncY1: number;
    let IncX2: number, IncY2: number;
    let Den: number;
    let Num: number;
    let NumAdd: number;
    let NumPix: number;

    if (x2 >= x1) {
      IncX1 = 1;
      IncX2 = 1;
    } else {
      IncX1 = -1;
      IncX2 = -1;
    }
    if (y2 >= y1) {
      IncY1 = 1;
      IncY2 = 1;
    } else {
      IncY1 = -1;
      IncY2 = -1;
    }
    if (dx >= dy) {
      IncX1 = 0;
      IncY2 = 0;
      Den = dx;
      Num = dx / 2;
      NumAdd = dy;
      NumPix = dx;
    } else {
      IncX2 = 0;
      IncY1 = 0;
      Den = dy;
      Num = dy / 2;
      NumAdd = dx;
      NumPix = dy;
    }

    NumPix = Math.round(Q.LastPx + NumPix);

    let i = Q.LastPx;
    for (; i < NumPix; i++) {
      Num += NumAdd;
      if (Num >= Den) {
        Num -= Den;
        x += IncX1;
        y += IncY1;
      }
      x += IncX2;
      y += IncY2;
    }
    Q.Lastx = x;
    Q.Lasty = y;
    Q.LastPx = NumPix;
  }
  // let logToConsole: (message: string) => void = (message: string): void => {
  //   console.log(message)
  // }

  let CalcCross:(V0: Float64Array, V1: Float64Array)=> Float64Array =(V0: Float64Array, V1: Float64Array):Float64Array=>{
    let Cross: Float64Array = new Float64Array(4);
    Cross[0] = V0[1]*V1[2] - V0[2]*V1[1];
    Cross[1] = V0[2]*V1[0] - V0[0]*V1[2];
    Cross[2] = V0[0]*V1[1] - V0[1]*V1[0];
    return Cross;
  }

  let CalcNormal:(V0: number[], V1: number[], V2: number[]) =>Float64Array =(V0: number[], V1: number[], V2: number[]) :Float64Array=>{
    let A:Float64Array = new Float64Array(4);
    let B:Float64Array=  new Float64Array(4);
    for (let i = 0; i < 3; i++) {
      A[i] = V0[i] - V1[i];
      B[i] = V2[i] - V1[i];
    }
    A = CalcCross(A, B);
    let Length = Math.sqrt(A[0]*A[0] + A[1]*A[1] + A[2]*A[2]);
    for (let i = 0; i < 3; i++) A[i] = A[i] / Length;
    A[3] = 1;
    return A;
  }

  // multiplies two matrices
  let MMulti:(M1: number[][], M2: number[][])=>number[][]=(M1: number[][], M2: number[][]):number[][] =>{
    let M:number[][] = [new Array(4),new Array(4),new Array(4),new Array(4)]
    let i = 0;
    let j = 0;
    for (; i < 4; i++) {
      j = 0;
      for (; j < 4; j++) {
        M[i][j] = M1[i][0] * M2[0][j] + M1[i][1] * M2[1][j] + M1[i][2] * M2[2][j] + M1[i][3] * M2[3][j];
      }
    }
    return M;
  }

  //multiplies matrix with vector
  let VMulti:(M: number[][], V: number[])=> number[]=(M: number[][], V: number[]):number[]=> {
    let Vect:number[] = new Array(4);
    let i = 0;
    for (;i < 4; i++) {
      Vect[i] = M[i][0] * V[0] + M[i][1] * V[1] + M[i][2] * V[2] + M[i][3] * V[3];
    }
    return Vect;
  }

  let VMulti2:(M: number[][], V: Float64Array)=> number[]=(M: number[][], V: Float64Array):number[]=> {
    let Vect:number[] = new Array(4);
    let i = 0;
    for (;i < 3; i++) Vect[i] = M[i][0] * V[0] + M[i][1] * V[1] + M[i][2] * V[2];
    return Vect;
  }

  // add to matrices
  let MAdd:(M1: number[][], M2: number[][])=>number[][]=(M1: number[][], M2: number[][]):number[][]=> {
    let M:number[][] = [new Array(4),new Array(4),new Array(4),new Array(4)]
    let i = 0;
    let j = 0;
    for (; i < 4; i++) {
      j = 0;
      for (; j < 4; j++) M[i][j] = M1[i][j] + M2[i][j];
    }
    return M;
  }

  let Translate:(M: number[][], Dx: number, Dy: number, Dz: number)=> number[][]=(M: number[][], Dx: number, Dy: number, Dz: number): number[][]=>{
    let T:number[][] = [
      [1,0,0,Dx],
      [0,1,0,Dy],
      [0,0,1,Dz],
      [0,0,0,1]
    ];
    return MMulti(T, M);
  }

  let RotateX:(M: number[][], Phi: number)=> number[][]= (M: number[][], Phi: number):number[][]=>{
    let a:number = Phi;
    a *= Math.PI / 180;
    let Cos:number = Math.cos(a);
    let Sin:number = Math.sin(a);
    let R:number[][] = [
      [1,0,0,0],
      [0,Cos,-Sin,0],
      [0,Sin,Cos,0],
      [0,0,0,1]
    ];
    return MMulti(R, M);
  }

  let RotateY:(M: number[][], Phi: number)=> number[][]=(M: number[][], Phi: number):number[][]=> {
    let a:number = Phi;
    a *= Math.PI / 180;
    let Cos:number = Math.cos(a);
    let Sin:number = Math.sin(a);
    let R:number[][] = [
      [Cos,0,Sin,0],
      [0,1,0,0],
      [-Sin,0,Cos,0],
      [0,0,0,1]
    ];
    return MMulti(R, M);
  }

  let RotateZ:(M: number[][], Phi: number)=>number[][]=(M: number[][], Phi: number):number[][]=> {
    let a:number = Phi;
    a *= Math.PI / 180;
    let Cos:number = Math.cos(a);
    let Sin:number = Math.sin(a);
    let R:number[][] = [
      [Cos,-Sin,0,0],
      [Sin,Cos,0,0],
      [0,0,1,0],
      [0,0,0,1]
    ];
    return MMulti(R, M);
  }

  let DrawQube:()=>void=():void=> {
    // calc current normals
    let CurN:number[][] = new Array(6);
    let i: number = 5;
    Q.LastPx = 0;
    for (; i > -1; i--) {
      CurN[i] = VMulti2(MQube, Q.Normal[i]);
    }
    if (CurN[0][2] < 0) {
      if (!Q.Line[0]) { DrawLine(Q.arr[0], Q.arr[1]); Q.Line[0] = true; };
      if (!Q.Line[1]) { DrawLine(Q.arr[1], Q.arr[2]); Q.Line[1] = true; };
      if (!Q.Line[2]) { DrawLine(Q.arr[2], Q.arr[3]); Q.Line[2] = true; };
      if (!Q.Line[3]) { DrawLine(Q.arr[3], Q.arr[0]); Q.Line[3] = true; };
    }
    if (CurN[1][2] < 0) {
      if (!Q.Line[2]) { DrawLine(Q.arr[3], Q.arr[2]); Q.Line[2] = true; };
      if (!Q.Line[9]) { DrawLine(Q.arr[2], Q.arr[6]); Q.Line[9] = true; };
      if (!Q.Line[6]) { DrawLine(Q.arr[6], Q.arr[7]); Q.Line[6] = true; };
      if (!Q.Line[10]) { DrawLine(Q.arr[7], Q.arr[3]); Q.Line[10] = true; };
    }
    if (CurN[2][2] < 0) {
      if (!Q.Line[4]) { DrawLine(Q.arr[4], Q.arr[5]); Q.Line[4] = true; };
      if (!Q.Line[5]) { DrawLine(Q.arr[5], Q.arr[6]); Q.Line[5] = true; };
      if (!Q.Line[6]) { DrawLine(Q.arr[6], Q.arr[7]); Q.Line[6] = true; };
      if (!Q.Line[7]) { DrawLine(Q.arr[7], Q.arr[4]); Q.Line[7] = true; };
    }
    if (CurN[3][2] < 0) {
      if (!Q.Line[4]) { DrawLine(Q.arr[4], Q.arr[5]); Q.Line[4] = true; };
      if (!Q.Line[8]) { DrawLine(Q.arr[5], Q.arr[1]); Q.Line[8] = true; };
      if (!Q.Line[0]) { DrawLine(Q.arr[1], Q.arr[0]); Q.Line[0] = true; };
      if (!Q.Line[11]) { DrawLine(Q.arr[0], Q.arr[4]); Q.Line[11] = true; };
    }
    if (CurN[4][2] < 0) {
      if (!Q.Line[11]) { DrawLine(Q.arr[4], Q.arr[0]); Q.Line[11] = true; };
      if (!Q.Line[3]) { DrawLine(Q.arr[0], Q.arr[3]); Q.Line[3] = true; };
      if (!Q.Line[10]) { DrawLine(Q.arr[3], Q.arr[7]); Q.Line[10] = true; };
      if (!Q.Line[7]) { DrawLine(Q.arr[7], Q.arr[4]); Q.Line[7] = true; };
    }
    if (CurN[5][2] < 0) {
      if (!Q.Line[8]) { DrawLine(Q.arr[1], Q.arr[5]); Q.Line[8] = true; };
      if (!Q.Line[5]) { DrawLine(Q.arr[5], Q.arr[6]); Q.Line[5] = true; };
      if (!Q.Line[9]) { DrawLine(Q.arr[6], Q.arr[2]); Q.Line[9] = true; };
      if (!Q.Line[1]) { DrawLine(Q.arr[2], Q.arr[1]); Q.Line[1] = true; };
    }
    Q.Line = [false,false,false,false,false,false,false,false,false,false,false,false];
    Q.LastPx = 0;
  }

  let Loop:()=>void=():void=>{
    if (Testing.LoopCount > Testing.LoopMax) {
      return;
    }
    let TestingStr = String(Testing.LoopCount);
    while (TestingStr.length < 3) {
      TestingStr = "0" + TestingStr;
    }
    let MTrans:number[][] = Translate(I, -Q.arr[8].V[0], -Q.arr[8].V[1], -Q.arr[8].V[2]);
    MTrans = RotateX(MTrans, 1);
    MTrans = RotateY(MTrans, 3);
    MTrans = RotateZ(MTrans, 5);
    MTrans = Translate(MTrans, Q.arr[8].V[0], Q.arr[8].V[1], Q.arr[8].V[2]);
    MQube = MMulti(MTrans, MQube);
    let i = 8;
    for (; i > -1; i--) {
      Q.arr[i].V = VMulti(MTrans, Q.arr[i].V);
    }
    DrawQube();
    Testing.LoopCount++;
    Loop();
  }

  let Init:(CubeSize: number)=>void=(CubeSize: number):void=> {
    // init/reset vars
    Origin = [150,150,20,1];
    Testing.LoopCount = 0;
    Testing.LoopMax = 50;

    MTrans = [
      [1,0,0,0],
      [0,1,0,0],
      [0,0,1,0],
      [0,0,0,1]
    ]; // transformation matrix

    MQube = [
      [1,0,0,0],
      [0,1,0,0],
      [0,0,1,0],
      [0,0,0,1]
    ]; // position information of qube

    I = [
      [1,0,0,0],
      [0,1,0,0],
      [0,0,1,0],
      [0,0,0,1]
    ]; // entity matrix

    // create qube
    Q.arr[0] = new Px(-CubeSize,-CubeSize, CubeSize);
    Q.arr[1] = new Px(-CubeSize, CubeSize, CubeSize);
    Q.arr[2] = new Px( CubeSize, CubeSize, CubeSize);
    Q.arr[3] = new Px( CubeSize,-CubeSize, CubeSize);
    Q.arr[4] = new Px(-CubeSize,-CubeSize,-CubeSize);
    Q.arr[5] = new Px(-CubeSize, CubeSize,-CubeSize);
    Q.arr[6] = new Px( CubeSize, CubeSize,-CubeSize);
    Q.arr[7] = new Px( CubeSize,-CubeSize,-CubeSize);

    // center of gravity
    Q.arr[8] = new Px(0, 0, 0);
    // anti-clockwise edge check
    Q.Edge = [[0,1,2],[3,2,6],[7,6,5],[4,5,1],[4,0,3],[1,5,6]];

    // calculate squad normals
    Q.Normal = new Array(Q.Edge.length);
    for (let i = 0; i < Q.Edge.length; i++) Q.Normal[i] = CalcNormal(Q.arr[Q.Edge[i][0]].V, Q.arr[Q.Edge[i][1]].V, Q.arr[Q.Edge[i][2]].V);

    // line drawn ?
    Q.Line = [false,false,false,false,false,false,false,false,false,false,false,false];

    // create line pixels
    Q.NumPx = 9 * 2 * CubeSize;
    // for (let i = 0; i < Q.NumPx; i++) CreateP(0,0,0);

    MTrans = Translate(MTrans, Origin[0], Origin[1], Origin[2]);
    MQube = MMulti(MTrans, MQube);

    let i = 0;
    for (; i < 9; i++) {
      Q.arr[i].V = VMulti(MTrans, Q.arr[i].V);
    }
    DrawQube();
    Loop();

    // Perform a simple sum-based verification.
    let sum = 0;
    for (let i = 0; i < Q.arr.length; ++i) {
      let vector = Q.arr[i].V;
      for (let j = 0; j < vector.length; ++j)
        sum += vector[j];
    }
    let expected: number = 0;
    if (CubeSize === 20) {
      expected = validation.get(CubeSize);
    } else if (CubeSize === 40) {
      expected = validation.get(CubeSize);
    } else if (CubeSize === 80) {
      expected = validation.get(CubeSize);
    } else if (CubeSize === 160) {
      expected = validation.get(CubeSize);
    }
    if (sum != expected) {
      throw new Error("Error: bad vector sum for CubeSize = " + CubeSize + "; expected " + expected + " but got " + sum);
    }
  }

  let iter = 0;
  while (iter <= 10) {
    let i=20
    while (i <= 160) {
      Init(i)
      i *= 2
    }
    iter++;
  }
}

export function RunThreeDCube() {
  let start = ArkTools.timeInUs();
  run();
  let end = ArkTools.timeInUs();
  let time = (end - start) / 1000
  print("Array Access - RunThreeDCube:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunThreeDCube()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunThreeDCube()
// let runner = new BenchmarkRunner("Array Access - RunThreeDCube", RunThreeDCube);
// runner.run();
