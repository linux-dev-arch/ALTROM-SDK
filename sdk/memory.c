#include <stddef.h>
#include <stdint.h>

__attribute__((used)) void *memset(void *dest, int value, size_t n)
{
    uint8_t *d = (uint8_t *)dest;

    while (n--)
        *d++ = (uint8_t)value;

    return dest;
}

__attribute__((used)) int memcmp(const void *a, const void *b, size_t n)
{
    const uint8_t *x = (const uint8_t *)a;
    const uint8_t *y = (const uint8_t *)b;

    while (n--)
    {
        if (*x != *y)
            return (int)*x - (int)*y;

        x++;
        y++;
    }

    return 0;
}
__attribute__((used)) void __aeabi_memclr4(void *dest, unsigned int n)
{
    memset(dest, 0, n);
}
