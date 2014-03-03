#pragma once

#define PIC_VID                                 0x04D8
#define PIC_PID                                 0x1001
//#define USE_USB_NONBLOCKING
#define USB_NONBLOCKING_RETRY_COUNT             500
#define USB_NONBLOCKING_NEXT_RETRY_DELAY_MS     10
#define USB_READ_TIMEOUT_MS                     -1

#define I2C_HIH_DEVICE_ID                       0x27
#define STR_I2C_HIH_DEVICE_NAME                 "/dev/i2c-1"

#define MEAN_AND_VARIANCE_PERIOD_MS             300000   // 5 minuti
#define RESEND_PERIOD_MS                        7200000  // 2 ore
#define DUST_MIN_VALUE_PERIOD_MS                86400000 // 24 ore
#define DUST_MIN_INIT_VALUE                     std::numeric_limits<double>::max()
#define DUST_OFFSET_INIT_VALUE                  std::numeric_limits<double>::max()
#define STR_DIGEST_AUTH_USERNAME                "gruppo1"
#define STR_DIGEST_AUTH_PASSWORD                "VpNXXV3OEn"
#define HTTP_RES_CODE_OK                        200
#define SMTP_RES_CODE_OK                        250
#define HUMIDITY_AND_TEMPERATURE_ERR_CODE       0xFFFF
#define DUST_MEASUREMENT_PERIOD_MS              10
#define HUMIDITY_AND_TEMPERATURE_PERIOD_MS      1000
#define HUMIDITY_AND_TEMPERATURE_PERIOD_COUNTER HUMIDITY_AND_TEMPERATURE_PERIOD_MS / DUST_MEASUREMENT_PERIOD_MS

#define STR_RASPBERRY_MAC_ADDRESS               "b8:27:eb:69:a4:19"
#define RASPBERRY_LOCAL_FEED_ID_DUST            "0"
#define RASPBERRY_LOCAL_FEED_ID_IH              "1"
#define RASPBERRY_LOCAL_FEED_ID_IT              "2"
#define RASPBERRY_LOCAL_FEED_ID_EH              "3"
#define RASPBERRY_LOCAL_FEED_ID_ET              "4"

#define CROWDSENSING_TEST_APIS_URL              "http://crowdsensing.ismb.it/SC/rest/test-apis/device/b8:27:eb:69:a4:19/posts"
#define CROWDSENSING_APIS_URL                   "http://crowdsensing.ismb.it/SC/rest/apis/device/b8:27:eb:69:a4:19/posts"
#define CROWDSENSING_APIS_SEND_VALUE_MAX_RETRY  5 // Numero massimo di retry, per l'invio di una misura, in caso di fallimento al primo tentativo

#define RASPBERRY_DUST_UNIT_OF_MEASUREMENT      "MicrogramsPerCubicMeter"
#define RASPBERRY_IH_UNIT_OF_MEASUREMENT        "Percentage"
#define RASPBERRY_IT_UNIT_OF_MEASUREMENT        "Celsius"
#define RASPBERRY_EH_UNIT_OF_MEASUREMENT        "Percentage"
#define RASPBERRY_ET_UNIT_OF_MEASUREMENT        "Celsius"

#define STR_JSON_RASPB_WIFI_MAC_FORMAT          "raspb_wifi_mac"
#define STR_JSON_MAC_ADDRESS_FORMAT             "macAddress"
#define STR_JSON_SIGNAL_STRENGTH_FORMAT         "signalStrength"
#define STR_JSON_AGE_FORMAT                     "age"
#define STR_JSON_CHANNEL_FORMAT                 "channel"
#define STR_JSON_SIGNAL_TO_NOISE_RATIO_FORMAT   "signalToNoiseRatio"
#define STR_JSON_SEND_TIMESTAMP_FORMAT          "send_timestamp"
#define STR_JSON_VALUE_TIMESTAMP_FORMAT         "value_timestamp"
#define STR_JSON_KIND_FORMAT                    "kind"
#define STR_JSON_TIMESTAMPMS_FORMAT             "timestampMs"
#define STR_JSON_LATITUDE_FORMAT                "latitude"
#define STR_JSON_LONGITUDE_FORMAT               "longitude"
#define STR_JSON_ACCURACY_FORMAT                "accuracy"
#define STR_JSON_HEIGHT_METERS_FORMAT           "height_meters"
#define STR_JSON_POSITION_FORMAT                "position"
#define STR_JSON_SENSOR_VALUES_FORMAT           "sensor_values"
#define STR_JSON_AVERAGE_VALUE_FORMAT           "average_value"
#define STR_JSON_VARIANCE_FORMAT                "variance"
#define STR_JSON_UNITS_OF_MEASUREMENT_FORMAT    "units_of_measurement"
#define STR_JSON_LOCAL_FEED_ID_FORMAT           "local_feed_id"

#define MEASUREMENTS_TO_SEND_FILE_NAME          "./MeasurementsToSend.dat"   // File per le misure di cui ancora si devono fare dei tentativi di invio
#define UNSENDED_MEASUREMENTS_FILE_NAME         "./UnsendedMeasurements.dat" // File per le misure il cui invio e' fallito per tutti i retry disponibili

#define FROM                                    "<raspberry_gruppo_1@email.it>"
#define TO_ALESSIO_VALLERO                      "\"Alessio Vallero\" <s192474@studenti.polito.it>"
#define TO_FRANCESCO_PACE                       "\"Francesco Pace\" <s192462@studenti.polito.it>"
#define TO_STEFANO_MARTINALLO                   "\"Stefano Martinallo\" <s192470@studenti.polito.it>"
#define TO_ALESSIO_PIERO_OTTOBONI               "\"Alessio Piero Ottoboni\" <s196992@studenti.polito.it>"
#define STR_EMAIL_SMTP                          "smtp://smtp.email.it:587"
#define STR_EMAIL_SUBJECT                       "Report di errori sul Raspberry"
#define STR_EMAIL_BODY_PREFIX                   "Sono stati rilevati i seguenti errori:"
#define STR_EMAIL_USERNAME                      "raspberry_gruppo_1@email.it"
#define STR_EMAIL_PASSWORD                      "Gruppo1Pi"

#define MAX_QUEUED_MESSAGES_BEFORE_EMAIL        20
#define MAX_QUEUED_MESSAGES_CAUSED_BY_ERRORS    100

//#define USE_GEOLOCATION_FROM_GOOGLE_API

// Macro per creare un char[] da una string, usando uno smart pointer unique
#define STRING_TO_CHARARRAY( stringIn , chararrayOut ) \
        unique_ptr<char[]> pUniquePtrChars( new char[ stringIn.size() + 1 ] ) ; \
        strcpy( pUniquePtrChars.get() , stringIn.c_str() ) ; \
        chararrayOut = pUniquePtrChars.get()

// Macro per creare una std::string partendo dall'oggetto JSON di JsonBox
#define JSON_TO_STRING( jsonIn , stringOut ) \
        std::stringstream ss ; \
        jsonIn.writeToStream( ss , false , false ) ; \
        stringOut = ss.str()

namespace Raspberry_Gruppo_1
{
    class CUtils
    {
        public:
            static bool ReadUnsendedMeasurementsFromFile() ;
            static bool WriteUnsendedMeasurementsToFile( string sFilePath , string& sUnsendedMeasurementData , int nRetryCount ) ;
            static bool ClearUnsendedMeasurementsFile() ;

            static double FormatDustValue( uint16_t nsDustTicks ) ;
            static double FormatHumidityValue( uint16_t nsHumidityTicks ) ;
            static double FormatTemperatureValue( uint16_t nsTemperatureTicks ) ;

            static string GetCurrentDatetimeISO8601Formatted() ;

            // Aggiunge un messaggio a m_arsWarningMessages
            static bool AddWarningMessage( string sWarningMessage ) ;
            // Aggiunge un messaggio a m_arsWarningMessages ed invio subito una email di report
            static bool AddWarningMessage( string sWarningMessage , bool bSendEmail ) ;
            static void ClearWarningMessages() ;
            static vector<string> m_arsWarningMessages ;

        private:
            static std::mutex m_Mutex ; // Variabile di mutua esclusione per l'accesso al vettore dei messaggi
    };
};
