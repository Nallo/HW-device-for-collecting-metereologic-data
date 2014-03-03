#pragma once

namespace Raspberry_Gruppo_1
{
    class CJSONUtils
    {
        public:
            static void setJSONFormattedGeolocation( Value jsonFormattedGeolocation ) ;
            static std::string CreateJSONGeolocationToSend() ;
            static std::string CreateGeolocationPOSTData() ;
            static bool CreateAndSetGeolocationData() ;
            static std::string CreateSensorPostPOSTData( t_MeasurementData measurementDataMedia , t_MeasurementData measurementDataVarianza ) ;

        private:
            static Value m_JSONFormattedGeolocation ;
    };
};
