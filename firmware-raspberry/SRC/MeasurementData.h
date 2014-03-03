#pragma once

namespace Raspberry_Gruppo_1
{
    struct t_HumidityAndTemperatureMeasurementData
    {
        double m_dHumidityPercentage ;
        double m_dTemperatureCelsius ;
    };

    struct t_MeasurementData
    {
        double m_dExternalDust ;

        struct t_HumidityAndTemperatureMeasurementData m_InternalHumidityAndTemperature ;
        struct t_HumidityAndTemperatureMeasurementData m_ExternalHumidityAndTemperature ;
    };

    struct t_MeasurementDataToResend
    {
        std::string m_sMeasurementDataToResend ;
        int m_nRetryCount ;
    };
};
