#pragma once

namespace Raspberry_Gruppo_1
{
    class CUsbUtils
    {
        public:
            CUsbUtils() ;
            ~CUsbUtils() ;

            bool Read( unsigned char* pReadBuffer , int nSize , int* nReadedBytes ) ;

        private:
            void Init() ;

            hid_device* m_pDeviceHandle ;
    };
};
