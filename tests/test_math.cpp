#include <assert.h>
#include "../engine/math.inl"
#include <limits>

void test_approach() {
    int i = math::approach(1, 10, 2);
    assert(i == 3);

    i = math::approach(10, 4, 1);
    assert(i == 9);

    i = math::approach(10, -3, 10);
    assert(i == 0);

    i = math::approach(20, 30, 100);
    assert(i == 30);

    float f = math::approach(1.0f, 10.0f, 2.0f);
    assert(abs(3.0f - f) < std::numeric_limits<double>::epsilon());
}

int main(int, char**) {
    test_approach();

    return 0;
}
