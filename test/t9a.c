#define MSETLEN		(1 << 15)

int main(void)
{
	int a = 40;
	int d = 32772;
	unsigned x = 8;
	if (a + 8u > MSETLEN - d)
		return 1;
	if (a + x > MSETLEN - d)
		return 2;
	if (8u + a > MSETLEN - d)
		return 3;
	if (x + a > MSETLEN - d)
		return 4;
	return 0;
}
