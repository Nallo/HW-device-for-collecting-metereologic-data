#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <limits>

#include "MeasurementData.h"
#include "tbb/tbb.h"
#include "tbb/concurrent_queue.h"
#include "LibcurlUtils.h"

using namespace std ;

#include "Utils.h"

#include "ResendValuesManager.h"

using namespace tbb ;
using namespace Raspberry_Gruppo_1 ;

extern std::atomic<bool> bContinueExecution ;
concurrent_queue<t_MeasurementDataToResend> valuesToResend ;


void CResendValuesManager::operator()()
{
    // Caricamento di eventuali misure dal file di backup
    CUtils::ReadUnsendedMeasurementsFromFile() ;

    while( bContinueExecution )
    {
        ResendValues() ;

        // Prova a reinviare ogni 2 ore
        chrono::milliseconds sleep_duration( RESEND_PERIOD_MS ) ;
        this_thread::sleep_for( sleep_duration ) ;
    }
}

void CResendValuesManager::ResendValues()
{
    try
    {
        if( CUtils::ClearUnsendedMeasurementsFile() )
        {
            // Si utilizzano le size non thread-safe (unica funzione che restituisce la size della concurrent_queue al momento),
            // poiche', da specifica, ritornano l'ultimo valore aggiornato di push - pop fatte, utilizzando variabili locali
            // che non possono esser soggette a problemi in caso di nuove push o pop in contemporanea da altri thread.
            // (vedere anche il commento degli sviluppatori di Intel su http://software.intel.com/en-us/forums/topic/286212)
            size_t nValuesToResendSize = valuesToResend.unsafe_size() ;

            // Dati da reinviare
            // (non sarebbe utile un parallel_for perche', al momento, ci aspettiamo che questo sia un for con pochi valori)
            for ( size_t i = 0 ; i < nValuesToResendSize ; i++ )
            {
                t_MeasurementDataToResend valueToResend ;

                if ( valuesToResend.try_pop( valueToResend ) )
                {
                    try
                    {
                        STRING_TO_CHARARRAY( valueToResend.m_sMeasurementDataToResend , char* arcPostField ) ;
                        if ( !CLibcurlUtils::CreatePOSTData( true , valueToResend.m_nRetryCount - 1 , NULL , arcPostField , CROWDSENSING_APIS_URL ) )
                        {
                            valueToResend.m_nRetryCount-- ;
                            if ( valueToResend.m_nRetryCount == 0 )
                            {
                                // Se l'esecuzione del comando di invio dei dati e' fallito, e ho finito i retry, invio una email di report
                                CUtils::AddWarningMessage( "CResendValuesManager::ResendValues - Fallito l'invio del dato: " + valueToResend.m_sMeasurementDataToResend , true ) ;

                                // Scrittura sul file di backup per le misure il cui invio non e' riuscito
                                CUtils::WriteUnsendedMeasurementsToFile( UNSENDED_MEASUREMENTS_FILE_NAME , valueToResend.m_sMeasurementDataToResend , valueToResend.m_nRetryCount ) ;
                            }
                            else
                            {
                                // Non sono finiti i retry, rimetto in coda
                                valuesToResend.push( valueToResend ) ;

                                // Scrittura anche sul file delle misure che si riprovera' ad inviare
                                CUtils::WriteUnsendedMeasurementsToFile( MEASUREMENTS_TO_SEND_FILE_NAME , valueToResend.m_sMeasurementDataToResend , valueToResend.m_nRetryCount ) ;
                            }
                        }
                    }
                    catch( const exception& e )
                    {
                        CUtils::AddWarningMessage( "CResendValuesManager::ResendValues - Eccezione generica. Testo: " + string( e.what() ) ) ;
                    }
                }
                else
                {
                    CUtils::AddWarningMessage( "CResendValuesManager::ResendValues - Fallito il tentativo di ottenere un elemento da reinviare!" ) ;
                }   
            }
        }
    }
    catch( const exception& e )
    {
        CUtils::AddWarningMessage( "CResendValuesManager::ResendValues - Eccezione durante il tentativo di reinvio. Testo eccezione: " + string( e.what() ) ) ;
    }
}
