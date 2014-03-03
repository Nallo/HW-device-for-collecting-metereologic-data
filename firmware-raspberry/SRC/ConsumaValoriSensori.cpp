#include <iostream>
#include <vector>
#include <atomic>
#include <cmath>
#include <chrono>
#include <limits>
#include <thread>
#include <future>
#include <mutex>
#include "MeasurementData.h"
#include "tbb/tbb.h"
#include "tbb/concurrent_queue.h"
#include "I2C_HIH_Measurement.h"
#include "JsonBox/include/JsonBox/Value.h"

using namespace JsonBox ;

#include "JSONUtils.h"
#include "LibcurlUtils.h"

using namespace std ;

#include "Utils.h"

#include "ConsumaValoriSensori.h"
#include "TaratoreValoriPolveri.h"
#include "ResendValuesManager.h"

using namespace tbb ;
using namespace Raspberry_Gruppo_1 ;

extern std::atomic<double> dExternalDustCurrentOffset ; 
extern concurrent_queue<double> externalDustMeasurements ;
extern concurrent_queue<t_HumidityAndTemperatureMeasurementData> internalHumidityAndTemperatureMeasurements ;
extern concurrent_queue<t_HumidityAndTemperatureMeasurementData> externalHumidityAndTemperatureMeasurements ;

extern std::atomic<bool> bContinueExecution ;


void CConsumaValoriSensori::operator()()
{
#ifdef USE_GEOLOCATION_FROM_GOOGLE_API
    // Crea il thread per la richiesta della geolocazione
    // async per permettere a questo thread di continuare anche durante la richiesta
    future<void> threadGeolocationRequest = async( launch::async , CLibcurlUtils::GeolocationRequest ) ;

    // Prima sleep, messa fuori dal ciclo while in modo da poter sapere l'esito della richiesta di geolocazione
    // prima di entrarci
    chrono::milliseconds sleep_duration( MEAN_AND_VARIANCE_PERIOD_MS ) ;
    this_thread::sleep_for( sleep_duration ) ;

    threadGeolocationRequest.wait() ;

    // Crea il thread che gestisce il reinvio dei valori rilevati dai sensori che non sono riusciti subito ad esser inviati
    thread threadResendValuesManager( ( CResendValuesManager() ) ) ;
    // Detach perche' la sua esecuzione puo' viaggiare indipendente e noi non dobbiamo conoscerne lo stato
    threadResendValuesManager.detach() ;

    while( bContinueExecution )
    {        
        CalculateMeanAndVariance() ;

        // Processa i valori ogni 5 minuti
        chrono::milliseconds sleep_duration( MEAN_AND_VARIANCE_PERIOD_MS ) ;
        this_thread::sleep_for( sleep_duration ) ;
    }
#else // USE_GEOLOCATION_FROM_GOOGLE_API
    if ( CJSONUtils::CreateAndSetGeolocationData() )
    {
        // Crea il thread che gestisce il reinvio dei valori rilevati dai sensori che non sono riusciti subito ad esser inviati
        thread threadResendValuesManager( ( CResendValuesManager() ) ) ;
        // Detach perche' la sua esecuzione puo' viaggiare indipendente e noi non dobbiamo conoscerne lo stato
        threadResendValuesManager.detach() ;

        while( bContinueExecution )
        {        
            // Processa i valori ogni 5 minuti
            chrono::milliseconds sleep_duration( MEAN_AND_VARIANCE_PERIOD_MS ) ;
            this_thread::sleep_for( sleep_duration ) ;

            CalculateMeanAndVariance() ;
        }
    }
    else
    {
        // Se l'esecuzione del comando di richiesta della geolocazione e' fallito, invio una email di report ed interrompo l'esecuzione del programma
        CUtils::AddWarningMessage( "CConsumaValoriSensori::operator() - Fallita la creazione della stringa di geolocazione." , true ) ;
        bContinueExecution = false ;
    }
#endif // USE_GEOLOCATION_FROM_GOOGLE_API
}

void CConsumaValoriSensori::CalculateMeanAndVariance()
{
    try
    {
        double dExternalDustSum = 0.0 ;
        double dInternalHumidityPercentageSum = 0.0 ;
        double dInternalTemperatureCelsiusSum = 0.0 ;
        double dExternalHumidityPercentageSum = 0.0 ;
        double dExternalTemperatureCelsiusSum = 0.0 ;

        double dExternalDustQuadraticSum = 0.0 ;
        double dInternalHumidityPercentageQuadraticSum = 0.0 ;
        double dInternalTemperatureCelsiusQuadraticSum = 0.0 ;
        double dExternalHumidityPercentageQuadraticSum = 0.0 ;
        double dExternalTemperatureCelsiusQuadraticSum = 0.0 ;

        // Si utilizzano le size non thread-safe (unica funzione che restituisce la size della concurrent_queue al momento),
        // poiche', da specifica, ritornano l'ultimo valore aggiornato di push - pop fatte, utilizzando variabili locali
        // che non possono esser soggette a problemi in caso di nuove push o pop in contemporanea da altri thread.
        // (vedere anche il commento degli sviluppatori di Intel su http://software.intel.com/en-us/forums/topic/286212)
        // Pertanto, nel caso arrivino altri valori durante i calcoli di media e varianza, non ci sarebbero comunque problemi,
        // semplicemente quei valori verrebbero processati al prossimo invio.
        // E' preferibile questa scelta rispetto ad un ciclo la cui condizione si basa sul fatto che la coda sia vuota,
        // per il seguente motivo: noi per il sensore delle polveri inseriamo valori ogni 10 ms; se per caso un domani si
        // elaborassero tali valori con tempi inferiori, potrebbe capitare (sebbene con probabilita' estremamente bassa) che il
        // ciclo non abbia ancora finito l'iterazione mentre il thread esterno che inserisce i valori abbia inserito un nuovo
        // valore, esponendoci al rischio di un loop infinito.
        size_t nExternalDustMeasurementsSize = externalDustMeasurements.unsafe_size() ;
        size_t nInternalHumidityAndTemperatureMeasurementsSize = internalHumidityAndTemperatureMeasurements.unsafe_size() ;
        size_t nExternalHumidityAndTemperatureMeasurementsSize = externalHumidityAndTemperatureMeasurements.unsafe_size() ;
        
        // Se non e' mai stato calcolato l'offset, lo calcolo ora per la prima volta
        if ( dExternalDustCurrentOffset == DUST_OFFSET_INIT_VALUE )
        {
            CTaratoreValoriPolveri::CalculateDustOffset() ;
        }

        // Salvataggio su variabile locale dell'offset, in modo da non leggere in continuazione su una variabile concorrente nel prossimo ciclo for;
        // sarebbe infatti piu' onerosa ciascuna lettura (eventuali variazioni dell'offset durante il ciclo for non ci interessano, poiche' l'offset
        // da usare e' comunque questo, dato che e' ricavato proprio sui valori che andiamo a processare tra poco)
        double dExternalDustOffset = dExternalDustCurrentOffset ;

        // Dati per la media e la varianza delle polveri
        parallel_for ( size_t( 0 ) , nExternalDustMeasurementsSize , size_t( 1 ) , [&]( size_t i )
        {
            double dExternalDustElement ;
            if ( externalDustMeasurements.try_pop( dExternalDustElement ) )
            {
                dExternalDustElement += dExternalDustOffset ;

                // Se il valore e' minore di zero, allora aggiorniamo l'offset (poiche' riteniamo probabile che le prossime misure imminenti siano
                // anch'esse inferiori a zero per un certo periodo) e consideriamo il valore comunque equivalente a 0
                if ( dExternalDustElement < 0.0 )
                {
                    dExternalDustCurrentOffset = dExternalDustCurrentOffset - dExternalDustElement ;
                    dExternalDustOffset = dExternalDustCurrentOffset ;
                    dExternalDustElement = 0.0 ;
                }

                //cout << "Polvere: " << dExternalDustElement << endl ;

                dExternalDustSum += dExternalDustElement ;
                dExternalDustQuadraticSum += pow( dExternalDustElement , 2.0 ) ;
            }
            else
            {
                CUtils::AddWarningMessage( "CConsumaValoriSensori::CalculateMeanAndVariance - Fallito il tentativo di ottenere un elemento per la media e la varianza delle polveri!" ) ;
            }            
        } ) ;
        // Dati per la media e la varianza di temperatura ed umidita' interne
        // (non sarebbe utile un parallel_for perche', al momento, ci aspettiamo che questo sia un for con pochi valori)
        for ( size_t i = 0 ; i < nInternalHumidityAndTemperatureMeasurementsSize ; i++ )
        {
            t_HumidityAndTemperatureMeasurementData internalHumidityAndTemperatureMeasurementsData ;

            if ( internalHumidityAndTemperatureMeasurements.try_pop( internalHumidityAndTemperatureMeasurementsData ) )
            {
                dInternalHumidityPercentageSum += internalHumidityAndTemperatureMeasurementsData.m_dHumidityPercentage ;
                dInternalTemperatureCelsiusSum += internalHumidityAndTemperatureMeasurementsData.m_dTemperatureCelsius ;
                dInternalHumidityPercentageQuadraticSum += pow( internalHumidityAndTemperatureMeasurementsData.m_dHumidityPercentage , 2.0 ) ;
                dInternalTemperatureCelsiusQuadraticSum += pow( internalHumidityAndTemperatureMeasurementsData.m_dTemperatureCelsius , 2.0 ) ;
            }
            else
            {
                CUtils::AddWarningMessage( "CConsumaValoriSensori::CalculateMeanAndVariance - Fallito il tentativo di ottenere un elemento per la media e la varianza di temperatura ed umidita' interne!" ) ;
            }   
        }
        // Dati per la media e la varianza di temperatura ed umidita' esterne
        // (non sarebbe utile un parallel_for perche', al momento, ci aspettiamo che questo sia un for con pochi valori)
        for ( size_t i = 0 ; i < nExternalHumidityAndTemperatureMeasurementsSize ; i++ )
        {
            t_HumidityAndTemperatureMeasurementData externalHumidityAndTemperatureMeasurementsData ;

            if ( externalHumidityAndTemperatureMeasurements.try_pop( externalHumidityAndTemperatureMeasurementsData ) )
            {
                dExternalHumidityPercentageSum += externalHumidityAndTemperatureMeasurementsData.m_dHumidityPercentage ;
                dExternalTemperatureCelsiusSum += externalHumidityAndTemperatureMeasurementsData.m_dTemperatureCelsius ;
                dExternalHumidityPercentageQuadraticSum += pow( externalHumidityAndTemperatureMeasurementsData.m_dHumidityPercentage , 2.0 ) ;
                dExternalTemperatureCelsiusQuadraticSum += pow( externalHumidityAndTemperatureMeasurementsData.m_dTemperatureCelsius , 2.0 ) ;
            }
            else
            {
                CUtils::AddWarningMessage( "CConsumaValoriSensori::CalculateMeanAndVariance - Fallito il tentativo di ottenere un elemento per la media e la varianza di temperatura ed umidita' esterne!" ) ;
            }   
        }

        t_MeasurementData measurementMeanData ;
        measurementMeanData.m_dExternalDust = dExternalDustSum / nExternalDustMeasurementsSize ;
        measurementMeanData.m_InternalHumidityAndTemperature.m_dHumidityPercentage = dInternalHumidityPercentageSum / nInternalHumidityAndTemperatureMeasurementsSize ;
        measurementMeanData.m_InternalHumidityAndTemperature.m_dTemperatureCelsius = dInternalTemperatureCelsiusSum / nInternalHumidityAndTemperatureMeasurementsSize ;
        measurementMeanData.m_ExternalHumidityAndTemperature.m_dHumidityPercentage = dExternalHumidityPercentageSum / nExternalHumidityAndTemperatureMeasurementsSize ;
        measurementMeanData.m_ExternalHumidityAndTemperature.m_dTemperatureCelsius = dExternalTemperatureCelsiusSum / nExternalHumidityAndTemperatureMeasurementsSize ;

        // La varianza e' la differenza fra la media quadratica al quadrato meno il quadrato della media aritmetica
        t_MeasurementData measurementVarianceData ;
        measurementVarianceData.m_dExternalDust = ( dExternalDustQuadraticSum / nExternalDustMeasurementsSize ) - pow( measurementMeanData.m_dExternalDust , 2.0 ) ;
        measurementVarianceData.m_InternalHumidityAndTemperature.m_dHumidityPercentage = ( dInternalHumidityPercentageQuadraticSum / nInternalHumidityAndTemperatureMeasurementsSize ) - pow( measurementMeanData.m_InternalHumidityAndTemperature.m_dHumidityPercentage , 2.0 ) ;
        measurementVarianceData.m_InternalHumidityAndTemperature.m_dTemperatureCelsius = ( dInternalTemperatureCelsiusQuadraticSum / nInternalHumidityAndTemperatureMeasurementsSize ) - pow( measurementMeanData.m_InternalHumidityAndTemperature.m_dTemperatureCelsius , 2.0 ) ;
        measurementVarianceData.m_ExternalHumidityAndTemperature.m_dHumidityPercentage = ( dExternalHumidityPercentageQuadraticSum / nExternalHumidityAndTemperatureMeasurementsSize ) - pow( measurementMeanData.m_ExternalHumidityAndTemperature.m_dHumidityPercentage , 2.0 ) ;
        measurementVarianceData.m_ExternalHumidityAndTemperature.m_dTemperatureCelsius = ( dExternalTemperatureCelsiusQuadraticSum / nExternalHumidityAndTemperatureMeasurementsSize ) - pow( measurementMeanData.m_ExternalHumidityAndTemperature.m_dTemperatureCelsius , 2.0 ) ;

        cout << "CConsumaValoriSensori::CalculateMeanAndVariance - INFO - Medie (dust inth intt exth extt): " << measurementMeanData.m_dExternalDust << " "
                                                                                                          << measurementMeanData.m_InternalHumidityAndTemperature.m_dHumidityPercentage << " "
                                                                                                          << measurementMeanData.m_InternalHumidityAndTemperature.m_dTemperatureCelsius << " "
                                                                                                          << measurementMeanData.m_ExternalHumidityAndTemperature.m_dHumidityPercentage << " "
                                                                                                          << measurementMeanData.m_ExternalHumidityAndTemperature.m_dTemperatureCelsius << endl ;
        cout << "CConsumaValoriSensori::CalculateMeanAndVariance - INFO - Varianze (dust inth intt exth extt): " << measurementVarianceData.m_dExternalDust << " "
                                                                                                          << measurementVarianceData.m_InternalHumidityAndTemperature.m_dHumidityPercentage << " "
                                                                                                          << measurementVarianceData.m_InternalHumidityAndTemperature.m_dTemperatureCelsius << " "
                                                                                                          << measurementVarianceData.m_ExternalHumidityAndTemperature.m_dHumidityPercentage << " "
                                                                                                          << measurementVarianceData.m_ExternalHumidityAndTemperature.m_dTemperatureCelsius << endl ;

        // Crea il thread che invia i valori via HTTP
        thread threadInvioHTTP( CLibcurlUtils::SendSensorPostPOSTData , false , measurementMeanData , measurementVarianceData ) ;
        // Detach perche' la sua esecuzione puo' viaggiare indipendente e noi non dobbiamo conoscerne lo stato
        threadInvioHTTP.detach() ;
    }
    catch ( const exception& e )
    {
        CUtils::AddWarningMessage( "CConsumaValoriSensori::CalculateMeanAndVariance - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }
}
