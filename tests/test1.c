struct myStruct
{
    uint8_t myVar;
};

//int getIncrementedNumber(int number)
//{
//    return number + 1;
//}

uint8_t incrementNumber(uint8_t num)
{
    return num + 1;
}

void main()
{
    uint8_t var1 = incrementNumber(3);
    uint8_t var2 = 2 + var1;
    //int var2 = getIncrementedNumber(4);
    //var2 = var2 - var1;;
}
