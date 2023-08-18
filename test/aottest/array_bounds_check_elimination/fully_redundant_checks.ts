function get(a: number[], p: number) {
    if(p <= a.length && p > 0) {
        return a[p - 1];
    }
    return 0;
}

function get2(a: number[], p: number) {
    if(p <= a.length - 1 && p >= 0) {
        return a[p];
    }
    return 0;
}

let a: number[] = [1, 2, 3];
let b = 2;
get(a, b);
get2(a, b);