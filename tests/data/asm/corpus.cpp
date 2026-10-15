extern "C" {
int simple_add(int a, int b) { return a + b; }

int if_else(int x) {
    if (x > 10) return x * 2;
    else if (x < -10) return -x;
    return 0;
}

int loop_sum(const int* a, int n) {
    int s = 0;
    for (int i = 0; i < n; ++i) s += a[i];
    return s;
}

int sw(int k) {
    switch (k) {
        case 0: return 11;
        case 1: return 22;
        case 2: return 33;
        case 3: return 44;
        case 4: return 55;
        case 5: return 66;
        default: return -1;
    }
}

int helper(int);
int tail_call(int x) { return helper(x + 1); }

int f0(int); int f1(int); int f2(int); int f3(int); int f4(int); int f5(int);
int sw_call(int k, int v) {
    switch (k) {
        case 0: return f0(v);
        case 1: return f1(v);
        case 2: return f2(v);
        case 3: return f3(v);
        case 4: return f4(v);
        case 5: return f5(v);
        default: return 0;
    }
}

int call_twice(int x) { return helper(x) + helper(x + 1); }

int indirect(int (*fp)(int), int v) { return fp(v); }

static int local_fn(int x) { return x * 3; }
int uses_local(int x) { return local_fn(x) + local_fn(x + 1); }
}
