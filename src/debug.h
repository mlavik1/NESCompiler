#ifndef CNES_DEBUG_H
#define CNES_DEBUG_H

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

#define DEBUG_MODE_NONE				0x0000
#define DEBUG_MODE_INFO				0x0001
#define DEBUG_MODE_WARNING			0x0002
#define DEBUG_MODE_EXCEPTION		0x0004
#define DEBUG_MODE_ERROR			0x0008
#define DEBUG_MODE_ALL				0x000F

typedef unsigned int DEBUG_MODE;
typedef unsigned int OUTPUT_MODE;

class Debug
{
public:
    Debug(DEBUG_MODE mode, const char* arg_file, int arg_line)
    {
        outputMode = mode;

        if (!(outputMode & (fileLogMode | terminalLogMode)))
            return;

        std::string prefix;
        bool printFileLineInfo = false;

        DEBUG_MODE bitmask = outputMode;
        DEBUG_MODE bit = DEBUG_MODE_ERROR;
        while (bitmask)
        {
            switch (bitmask & bit)
            {
            case DEBUG_MODE_INFO:
                prefix = "";
                break;
            case DEBUG_MODE_WARNING:
                prefix = "Warning: ";
                printFileLineInfo = true;
                break;
            case DEBUG_MODE_ERROR:
                prefix = "ERROR: ";
                printFileLineInfo = true;
                break;
            case DEBUG_MODE_EXCEPTION:
                prefix = "Exception: ";
                printFileLineInfo = true;
                break;
            }
            bitmask &= ~bit;
            bit >>= 1;
        }

        _buffer << prefix;
        if (printFileLineInfo)
            _buffersuffix << ", in " << arg_file << ", line " << arg_line;
    }

    template <typename T>
    Debug & operator<<(T const &value)
    {
        if (outputMode & terminalLogMode)
            _buffer << value;
        return *this;
    }

    ~Debug()
    {
        _buffer << _buffersuffix.str();
        _buffer << std::endl;
        std::cout << _buffer.str();


        if (fileLogMode & outputMode)
        {
            std::ofstream myFile("DebugLog.txt", firstTime ? std::ios::trunc : std::ios::app);
            if (firstTime)
                firstTime = false;

            myFile << _buffer.str();
            myFile.close();
        }

    }


    static void SetTerminalLogMode(DEBUG_MODE mode)
    {
        terminalLogMode = mode;
    }

    static void SetFileLogMode(DEBUG_MODE mode)
    {
        fileLogMode = mode;
    }

private:
    std::ostringstream _buffer;
    std::ostringstream _buffersuffix;
    static DEBUG_MODE terminalLogMode;
    static DEBUG_MODE fileLogMode;
    static DEBUG_MODE outputMode;
    static bool firstTime;
};


#define LOG(level) \
	Debug(level, __FILE__,__LINE__)
#define LOG_INFO() \
	Debug(DEBUG_MODE_INFO, __FILE__,__LINE__)
#define LOG_WARNING() \
	Debug(DEBUG_MODE_WARNING, __FILE__,__LINE__)
#define LOG_ERROR() \
	Debug(DEBUG_MODE_ERROR, __FILE__,__LINE__)
#define LOG_EXCEPTION() \
	Debug(DEBUG_MODE_EXCEPTION, __FILE__,__LINE__)


#endif
