#pragma once

namespace Raspberry_Gruppo_1
{
    struct EmailBodyUploadStatus
    {
        uint8_t m_nssIndiceRigheFisse ;
        uint8_t m_nssIndiceRigheCorpeEmail ;
    };

    class CLibcurlUtils
    {
        public:
            static void GeolocationRequest() ;
            static void SendSensorPostPOSTData( bool bTestHttp , t_MeasurementData measurementDataMedia , t_MeasurementData measurementDataVarianza ) ;
            static bool CreatePOSTData( bool bNeedAuthentication , int nCurrentRetryCount , void ( *pPostCallback )( void* , size_t , size_t , void* ) , char* arcPostFields , const char* arcURL ) ;

            static void SendEmailReport() ;
        private:
            // Funzione callback quando la richiesta di geolocazione e' terminata
            static void OnGeolocationRequested( void* ptr , size_t nSize , size_t nMemb , void* pStream ) ;
            // Funzione callback per poter processare il corpo del messaggio dell'email
            static size_t OnEmailReportReadData( void* ptr , size_t nSize , size_t nMemb , void* pEmailBodyUploadStatus ) ;

            // Intestazione fissa per la richiesta al server SMTP di posta
            static const char* arsFixedEmailPayload_Text[] ;
    };
};
