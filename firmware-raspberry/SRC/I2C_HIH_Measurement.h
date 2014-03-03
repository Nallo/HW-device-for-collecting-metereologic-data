#pragma once

namespace Raspberry_Gruppo_1
{
   
    class CI2C_HIH_Measurement
    {
    public:
        CI2C_HIH_Measurement() ;
        ~CI2C_HIH_Measurement() ;
    
        void ReadInternalHumidityAndInternalTemperature() ;

    private:
        void Init() ;

        int m_nI2CFileDescriptor ;
        bool m_bInited ;
    };
};
