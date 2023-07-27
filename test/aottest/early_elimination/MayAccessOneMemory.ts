let arraySize: number = 3
let arr:number[] = [1,2,3]
let res:Float64Array = new Float64Array([0,0,0])
for(let i = 0; i < 10; i++) { // cannot access one memony
    res[i % arraySize] = arr[i % 3];
    res[i % arraySize] = arr[i % 3];
}

let arr2:Float64Array = new Float64Array([0,0,0])
for(let i = 0; i < 10; i++) { // may access one memony
    res[i % arraySize] = arr2[i % 3];
    res[i % arraySize] = arr2[i % 3];
}