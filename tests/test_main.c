#include <criterion/criterion.h>
#include <criterion/new/assert.h>

Test(sample, adds_two_numbers) {
    int a = 2, b = 3;
    cr_assert(eq(int, a + b, 5));
}
