#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include "MeasurementData.h"
#include "tbb/tbb.h"
#include "tbb/concurrent_queue.h"
#include "JsonBox/include/JsonBox/Value.h"

using namespace JsonBox ;

#include "JSONUtils.h"
#include <curl/curl.h>
#include <curl/easy.h>

using namespace std ;

#include "Utils.h"

#include "ResendValuesManager.h"

#include "LibcurlUtils.h"

using namespace tbb ;
using namespace Raspberry_Gruppo_1 ;

extern atomic<bool> bContinueExecution ;
extern concurrent_queue<t_MeasurementDataToResend> valuesToResend ;


const char* CLibcurlUtils::arsFixedEmailPayload_Text[] =
{
    "To: " TO_ALESSIO_VALLERO ", " TO_FRANCESCO_PACE ", " TO_STEFANO_MARTINALLO ", " TO_ALESSIO_PIERO_OTTOBONI "\n" ,
    "From: " FROM "\n",
    "Subject: " STR_EMAIL_SUBJECT "\n",
    "\n", // Linea vuota per dividere headers da corpo del messaggio (previsto da RFC5322)
    STR_EMAIL_BODY_PREFIX "\n\n" ,
    NULL
};

void CLibcurlUtils::OnGeolocationRequested( void* ptr , size_t nSize , size_t nMemb , void* pStream )
{
    Value sensorGeolocationData ;
    sensorGeolocationData.loadFromString( string( (const char*)ptr ) ) ;
    CJSONUtils::setJSONFormattedGeolocation( sensorGeolocationData ) ;
}

size_t CLibcurlUtils::OnEmailReportReadData( void* ptr , size_t nSize , size_t nMemb , void* pEmailBodyUploadStatus )
{
    size_t nRet = 0 ;

    // Se non ho dati da trattare, esco
    if ( nSize == 0 || nMemb == 0 || (( nSize*nMemb ) < 1 ))
    {
        return nRet ;
    }

    struct EmailBodyUploadStatus* emailBodyUploadStatus = (struct EmailBodyUploadStatus *)pEmailBodyUploadStatus ;
    const char* pFixedData = arsFixedEmailPayload_Text[ emailBodyUploadStatus->m_nssIndiceRigheFisse ] ;

    // Se pFixedData non e' NULL, processiamo i dati fissi. Altrimenti i messaggi dinamici
    if ( pFixedData )
    {
        size_t nLen = strlen( pFixedData ) ;
        memcpy( ptr , pFixedData , nLen ) ;
        emailBodyUploadStatus->m_nssIndiceRigheFisse++ ;

        nRet = nLen ;
    }
    else
    {
        // Se non ho ancora finito di processare i dati dinamici, li processo
        if ( emailBodyUploadStatus->m_nssIndiceRigheCorpeEmail < CUtils::m_arsWarningMessages.size() )
        {
            size_t nDynamicContentSize = CUtils::m_arsWarningMessages[emailBodyUploadStatus->m_nssIndiceRigheCorpeEmail].size() ;
            memcpy( ptr, CUtils::m_arsWarningMessages[emailBodyUploadStatus->m_nssIndiceRigheCorpeEmail++].c_str() , nDynamicContentSize ) ;
            nRet = nDynamicContentSize ;
        }
    }

    return nRet ;
}

bool CLibcurlUtils::CreatePOSTData( bool bNeedAuthentication , int nCurrentRetryCount , void ( *pPostCallback )( void* , size_t , size_t , void* ) , char* arcPostFields , const char* arcURL )
{
    bool bRet = false ;

    // Inizializza la sessione curl
    CURL* pCurl_Handle = curl_easy_init() ;
    if ( pCurl_Handle )
    {
        // Imposta il tipo JSON alla POST
        struct curl_slist* pHeaders = NULL ;
        pHeaders = curl_slist_append( pHeaders , "Accept: application/json" ) ;
        pHeaders = curl_slist_append( pHeaders , "Content-Type: application/json" ) ;
        pHeaders = curl_slist_append( pHeaders , "charsets: utf-8" ) ;
        curl_easy_setopt( pCurl_Handle , CURLOPT_HTTPHEADER , pHeaders ) ;
        if ( bNeedAuthentication )
        {
            // Imposta i dati di autenticazione DIGEST
            curl_easy_setopt( pCurl_Handle , CURLOPT_HTTPAUTH , CURLAUTH_DIGEST ) ;
            curl_easy_setopt( pCurl_Handle , CURLOPT_USERNAME , STR_DIGEST_AUTH_USERNAME ) ;
            curl_easy_setopt( pCurl_Handle , CURLOPT_PASSWORD , STR_DIGEST_AUTH_PASSWORD ) ;
        }
        // Imposta la verbosita' della POST
        curl_easy_setopt( pCurl_Handle , CURLOPT_VERBOSE , 0 ) ;

        if ( pPostCallback != NULL )
        {
            // Imposta la callback della POST
            curl_easy_setopt( pCurl_Handle , CURLOPT_WRITEFUNCTION , pPostCallback ) ;
        }

        // Abilita il POST
        curl_easy_setopt( pCurl_Handle , CURLOPT_POST , 1 ) ;
        curl_easy_setopt( pCurl_Handle , CURLOPT_POSTFIELDS , arcPostFields ) ;
        // Imposta l'URL su cui fare il POST
        curl_easy_setopt( pCurl_Handle , CURLOPT_URL , arcURL ) ;
        // Setta il protocollo HTTP 1.1, in modo da esser sicuri che venga usato questo e non versioni precedenti
        curl_easy_setopt( pCurl_Handle , CURLOPT_HTTP_VERSION , CURL_HTTP_VERSION_1_1 ) ;
        // Imposta "l'inseguimento" dei redirect, nel caso il server venga spostato su un altro indirizzo e vi sia un redirect HTTP impostato
        curl_easy_setopt( pCurl_Handle , CURLOPT_FOLLOWLOCATION , 1 ) ;

        // Esegue il comando
        CURLcode res = curl_easy_perform( pCurl_Handle ) ;

        long nlHttpResCode = 0 ;
        curl_easy_getinfo( pCurl_Handle , CURLINFO_RESPONSE_CODE , &nlHttpResCode ) ;
        // Controllo degli errori
        if ( nlHttpResCode == HTTP_RES_CODE_OK )
        {
            bRet = true ;
        }
        else
        {
            CUtils::AddWarningMessage( string( "CLibcurlUtils::CreatePOSTData - curl_easy_perform fallita sull'indirizzo: " ) +
                              string( arcURL ) +
                              string( " HTTP Response: " ) +
                              to_string( nlHttpResCode ) +
                              string( " Contenuto del POST: " ) +
                              string( arcPostFields ) +
                              string( " Retry disponibili per questo contenuto: " ) +
                              to_string( nCurrentRetryCount ) +
                              string( " curl_easy_strerror: " ) +
                              curl_easy_strerror( res ) ) ;
        }

        // Libera le risorse usate da curl
        curl_easy_cleanup( pCurl_Handle ) ;
    }
    else
    {
        CUtils::AddWarningMessage( "CLibcurlUtils::CreatePOSTData - curl_easy_init fallita!" ) ;
    }

    return bRet ;
}

void CLibcurlUtils::GeolocationRequest()
{
    try
    {
        STRING_TO_CHARARRAY( CJSONUtils::CreateGeolocationPOSTData() , char* arcPostField ) ;
        if ( !CLibcurlUtils::CreatePOSTData( true , 0 , OnGeolocationRequested , arcPostField , "http://crowdsensing.ismb.it/SC/rest/apis/device/b8:27:eb:69:a4:19/geolocate" ) )
        {
            // Se l'esecuzione del comando di richiesta della geolocazione e' fallito, invio una email di report ed interrompo l'esecuzione del programma
            CUtils::AddWarningMessage( "CLibcurlUtils::GeolocationRequest - Fallita la richiesta." , true ) ;
            bContinueExecution = false ;
        }
    }
    catch( const exception& e )
    {
        CUtils::AddWarningMessage( "CLibcurlUtils::GeolocationRequest - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }
}

void CLibcurlUtils::SendSensorPostPOSTData( bool bTestHttp , t_MeasurementData measurementDataMedia , t_MeasurementData measurementDataVarianza )
{
    try
    {
        string sSensorPostPOSTData = CJSONUtils::CreateSensorPostPOSTData( measurementDataMedia , measurementDataVarianza ) ;
        STRING_TO_CHARARRAY( sSensorPostPOSTData , char* arcPostField ) ;
        if ( !CLibcurlUtils::CreatePOSTData( !bTestHttp ,
                                             CROWDSENSING_APIS_SEND_VALUE_MAX_RETRY ,
                                             NULL ,
                                             arcPostField ,
                                             bTestHttp ? CROWDSENSING_TEST_APIS_URL : CROWDSENSING_APIS_URL ) )
        {
            // Se l'esecuzione del comando di invio dei dati e' fallito, inserisco nella coda dei tentativi di reinvio
            t_MeasurementDataToResend measurementDataToResend ;
            measurementDataToResend.m_sMeasurementDataToResend = sSensorPostPOSTData ;
            measurementDataToResend.m_nRetryCount = CROWDSENSING_APIS_SEND_VALUE_MAX_RETRY ;

            valuesToResend.push( measurementDataToResend ) ;

            // Scrittura anche sul file di backup
            CUtils::WriteUnsendedMeasurementsToFile( MEASUREMENTS_TO_SEND_FILE_NAME , sSensorPostPOSTData , CROWDSENSING_APIS_SEND_VALUE_MAX_RETRY ) ;
        }
    }
    catch( const exception& e )
    {
        CUtils::AddWarningMessage( "CLibcurlUtils::SendSensorPostPOSTData - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }
}

void CLibcurlUtils::SendEmailReport()
{
    struct EmailBodyUploadStatus emailBodyUploadStatus ;
    emailBodyUploadStatus.m_nssIndiceRigheFisse = 0 ;
    emailBodyUploadStatus.m_nssIndiceRigheCorpeEmail = 0 ;

    CURL* pCurl_Handle = curl_easy_init() ;
    if ( pCurl_Handle )
    {
        /*
         * Indirizzo del mail server.
         * La porta 587 e' usata per utilizzare il secure mail submission (RFC4403)
         */
        curl_easy_setopt( pCurl_Handle , CURLOPT_URL , STR_EMAIL_SMTP ) ;

        // Uso del SSL
        curl_easy_setopt( pCurl_Handle , CURLOPT_USE_SSL , (long)CURLUSESSL_ALL ) ;

        curl_easy_setopt( pCurl_Handle , CURLOPT_USERNAME , STR_EMAIL_USERNAME ) ;
        curl_easy_setopt( pCurl_Handle , CURLOPT_PASSWORD , STR_EMAIL_PASSWORD ) ;

        curl_easy_setopt( pCurl_Handle , CURLOPT_MAIL_FROM , FROM ) ;

        // Destinatari
        struct curl_slist* arpRecipients = NULL ;
        arpRecipients = curl_slist_append( arpRecipients , TO_ALESSIO_VALLERO ) ;
        arpRecipients = curl_slist_append( arpRecipients , TO_FRANCESCO_PACE ) ;
        arpRecipients = curl_slist_append( arpRecipients , TO_STEFANO_MARTINALLO  ) ;
        arpRecipients = curl_slist_append( arpRecipients , TO_ALESSIO_PIERO_OTTOBONI ) ;

        curl_easy_setopt( pCurl_Handle , CURLOPT_MAIL_RCPT , arpRecipients ) ;

        // Callback con cui processare il BODY dell'email
        curl_easy_setopt( pCurl_Handle , CURLOPT_READFUNCTION , OnEmailReportReadData ) ;
        curl_easy_setopt( pCurl_Handle , CURLOPT_READDATA , &emailBodyUploadStatus ) ;

        /*
         * Siccome il traffico e' criptato, meglio aver traccia
         * di cosa succede durante lo scambio di messaggi del protocollo SMTP.
         */
        curl_easy_setopt( pCurl_Handle , CURLOPT_VERBOSE , 1L ) ;

        // Invio del messaggio
        CURLcode res = curl_easy_perform( pCurl_Handle ) ;

        long nlHttpResCode = 0 ;
        curl_easy_getinfo( pCurl_Handle , CURLINFO_RESPONSE_CODE , &nlHttpResCode ) ;
        // Controllo degli errori
        if ( nlHttpResCode == SMTP_RES_CODE_OK )
        {
             // Se tutto ok, cancello i messaggi dinamici in coda
             CUtils::ClearWarningMessages() ;
        }
        else
        {
            cout << curl_easy_strerror( res ) << endl ;
        }

        // Pulizia delle risorse utilizzate
        curl_slist_free_all( arpRecipients ) ;
        curl_easy_cleanup( pCurl_Handle ) ;
    }
    else
    {
        cout << "CLibcurlUtils::SendEmailReport - WARNING - curl_easy_init fallita!" << endl ;
    }
}
