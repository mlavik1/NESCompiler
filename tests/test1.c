struct myStruct
{
    uint8_t myVar;
};

uint8_t incrementNumber(uint8_t num)
{
    return num + 9;
}

void main()
{
    uint8_t var1 = incrementNumber(3);
    uint8_t var2 = 2 + var1;
}
