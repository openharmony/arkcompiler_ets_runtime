declare function print(arg:any):string;
class A {
    constructor() {
    }
    async g(x:number) {
        return x * 2;
    }
    async f(x:number) {
        await this.g(x * 2);
    }
}
