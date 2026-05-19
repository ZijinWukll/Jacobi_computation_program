#include "jacobi.h"
#include <algorithm>
using namespace std;

int jacobi(BigInt m, BigInt n) {
    int result = 1;
    while (!m.is_zero()) {
        int t = m.trailing_zeros();
        m.shr(t);
        if (t & 1) {
            int r8 = n.mod8();
            if (r8 == 3 || r8 == 5) result = -result;
        }
        if (m.is_one()) return result;
        if ((m.mod4() & n.mod4()) == 3) result = -result;
        swap(m, n);
        m.mod(n);
    }
    if (n.is_one()) return result;
    return 0;
}
