struct myStruct
{
    uint8_t myVar;
};

void playSound();

uint8_t incrementNumber(uint8_t num)
{
    return num + 9;
}

void main()
{
    uint8_t a = 2;
    uint8_t b = 1+2;
    if(a == b)
    {
        //playSound();
    }
    else
    {
        playSound();
    }
    
    uint8_t var1 = incrementNumber(3);
    uint8_t var2 = 2 + var1;
    //var2 = 4;
}
