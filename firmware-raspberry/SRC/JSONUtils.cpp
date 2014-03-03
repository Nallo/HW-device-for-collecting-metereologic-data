#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/time.h>
#include "JsonBox/include/JsonBox/Value.h"
#include "MeasurementData.h"

using namespace std ;
using namespace JsonBox ;

#include "Utils.h"
#include "JSONUtils.h"

using namespace JsonBox ;
using namespace Raspberry_Gruppo_1 ;


Value CJSONUtils::m_JSONFormattedGeolocation ;

void CJSONUtils::setJSONFormattedGeolocation( Value jsonFormattedGeolocation )
{
    CJSONUtils::m_JSONFormattedGeolocation = jsonFormattedGeolocation ;
}

string CJSONUtils::CreateJSONGeolocationToSend()
{
    string sRet = "" ;

    // Timestamp in millisecondi
    CJSONUtils::m_JSONFormattedGeolocation[STR_JSON_TIMESTAMPMS_FORMAT] = to_string( std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count() ) ;
    JSON_TO_STRING( CJSONUtils::m_JSONFormattedGeolocation , sRet ) ;

    return sRet ;
}

string CJSONUtils::CreateGeolocationPOSTData()
{
    string sRet = "" ;

    try
    {
        Value jsonGeolocationRequest ;
        // MAC Raspberry
        jsonGeolocationRequest[size_t(0)][STR_JSON_MAC_ADDRESS_FORMAT] = STR_RASPBERRY_MAC_ADDRESS ;
        // Potenza del segnale
        jsonGeolocationRequest[size_t(0)][STR_JSON_SIGNAL_STRENGTH_FORMAT] = Value() ;
        // Eta'
        jsonGeolocationRequest[size_t(0)][STR_JSON_AGE_FORMAT] = Value() ;
        // Canale
        jsonGeolocationRequest[size_t(0)][STR_JSON_CHANNEL_FORMAT] = Value() ;
        // Rumore segnale
        jsonGeolocationRequest[size_t(0)][STR_JSON_SIGNAL_TO_NOISE_RATIO_FORMAT] = Value() ;

        JSON_TO_STRING( jsonGeolocationRequest , sRet ) ;
    }
    catch( const exception& e )
    {
        CUtils::AddWarningMessage( "CJSONUtils::CreateGeolocationPOSTData - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }

    return sRet ;
}

bool CJSONUtils::CreateAndSetGeolocationData()
{
    bool bRet = false ;

    try
    {
        Value jsonGeolocationData ;
        // Tipo
        jsonGeolocationData[STR_JSON_KIND_FORMAT] = "latitude#location" ;
        // Timestamp in millisecondi
        jsonGeolocationData[STR_JSON_TIMESTAMPMS_FORMAT] = "" ;
        // Latitudine
        jsonGeolocationData[STR_JSON_LATITUDE_FORMAT] = 45.056606 ;
        // Longitudine
        jsonGeolocationData[STR_JSON_LONGITUDE_FORMAT] = 7.669717 ;
        // Precisione
        jsonGeolocationData[STR_JSON_ACCURACY_FORMAT] = 30.0 ; // metri
        // Altezza dal livello del mare
        jsonGeolocationData[STR_JSON_HEIGHT_METERS_FORMAT] = 242.0 ; // metri

        CJSONUtils::m_JSONFormattedGeolocation = jsonGeolocationData ;

        bRet = true ;
    }
    catch( const exception& e )
    {
        CUtils::AddWarningMessage( "CJSONUtils::CreateAndSetGeolocationData - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }

    return bRet ;
}

string CJSONUtils::CreateSensorPostPOSTData( t_MeasurementData measurementDataMedia , t_MeasurementData measurementDataVarianza )
{
    string sRet = "" ;

    try
    {
        // Concatenazione delle  due parti appena creazione, con i millisecondi in mezzo
        string sSendTimestamp = CUtils::GetCurrentDatetimeISO8601Formatted() ;

        Value sensorPostPOSTData ;
        // MAC Raspberry
        sensorPostPOSTData[STR_JSON_RASPB_WIFI_MAC_FORMAT] = STR_RASPBERRY_MAC_ADDRESS ;
        // Timestamp
        sensorPostPOSTData[STR_JSON_SEND_TIMESTAMP_FORMAT] = sSendTimestamp ;
        // Geolocalizzazione
        sensorPostPOSTData[STR_JSON_POSITION_FORMAT].loadFromString( CJSONUtils::CreateJSONGeolocationToSend() ) ;
        // Temperatura interna
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][4][STR_JSON_VALUE_TIMESTAMP_FORMAT] = sSendTimestamp ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][4][STR_JSON_AVERAGE_VALUE_FORMAT] = measurementDataMedia.m_InternalHumidityAndTemperature.m_dTemperatureCelsius ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][4][STR_JSON_VARIANCE_FORMAT] = measurementDataVarianza.m_InternalHumidityAndTemperature.m_dTemperatureCelsius ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][4][STR_JSON_UNITS_OF_MEASUREMENT_FORMAT] = RASPBERRY_IT_UNIT_OF_MEASUREMENT ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][4][STR_JSON_LOCAL_FEED_ID_FORMAT] = RASPBERRY_LOCAL_FEED_ID_IT ;
        // Umidita' interna
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][3][STR_JSON_VALUE_TIMESTAMP_FORMAT] = sSendTimestamp ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][3][STR_JSON_AVERAGE_VALUE_FORMAT] = measurementDataMedia.m_InternalHumidityAndTemperature.m_dHumidityPercentage ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][3][STR_JSON_VARIANCE_FORMAT] = measurementDataVarianza.m_InternalHumidityAndTemperature.m_dHumidityPercentage ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][3][STR_JSON_UNITS_OF_MEASUREMENT_FORMAT] = RASPBERRY_IH_UNIT_OF_MEASUREMENT ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][3][STR_JSON_LOCAL_FEED_ID_FORMAT] = RASPBERRY_LOCAL_FEED_ID_IH ;
        // Temperatura esterna
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][2][STR_JSON_VALUE_TIMESTAMP_FORMAT] = sSendTimestamp ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][2][STR_JSON_AVERAGE_VALUE_FORMAT] = measurementDataMedia.m_ExternalHumidityAndTemperature.m_dTemperatureCelsius ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][2][STR_JSON_VARIANCE_FORMAT] = measurementDataVarianza.m_ExternalHumidityAndTemperature.m_dTemperatureCelsius ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][2][STR_JSON_UNITS_OF_MEASUREMENT_FORMAT] = RASPBERRY_ET_UNIT_OF_MEASUREMENT ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][2][STR_JSON_LOCAL_FEED_ID_FORMAT] = RASPBERRY_LOCAL_FEED_ID_ET ;
        // Umidita' esterna
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][1][STR_JSON_VALUE_TIMESTAMP_FORMAT] = sSendTimestamp ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][1][STR_JSON_AVERAGE_VALUE_FORMAT] = measurementDataMedia.m_ExternalHumidityAndTemperature.m_dHumidityPercentage ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][1][STR_JSON_VARIANCE_FORMAT] = measurementDataVarianza.m_ExternalHumidityAndTemperature.m_dHumidityPercentage ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][1][STR_JSON_UNITS_OF_MEASUREMENT_FORMAT] = RASPBERRY_EH_UNIT_OF_MEASUREMENT ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][1][STR_JSON_LOCAL_FEED_ID_FORMAT] = RASPBERRY_LOCAL_FEED_ID_EH ;
        // Polvere
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][size_t(0)][STR_JSON_VALUE_TIMESTAMP_FORMAT] = sSendTimestamp ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][size_t(0)][STR_JSON_AVERAGE_VALUE_FORMAT] = measurementDataMedia.m_dExternalDust ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][size_t(0)][STR_JSON_VARIANCE_FORMAT] = measurementDataVarianza.m_dExternalDust ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][size_t(0)][STR_JSON_UNITS_OF_MEASUREMENT_FORMAT] = RASPBERRY_DUST_UNIT_OF_MEASUREMENT ;
        sensorPostPOSTData[STR_JSON_SENSOR_VALUES_FORMAT][size_t(0)][STR_JSON_LOCAL_FEED_ID_FORMAT] = RASPBERRY_LOCAL_FEED_ID_DUST ;

        JSON_TO_STRING( sensorPostPOSTData , sRet ) ;
    }
    catch( const exception& e )
    {
        CUtils::AddWarningMessage( "CJSONUtils::CreateSensorPostPOSTData - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }

    return sRet ;
}
