#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <mutex>
#include <vector>
#include <signal.h>
#include "MeasurementData.h"
#include "tbb/tbb.h"
#include "tbb/concurrent_queue.h"

using namespace std ;

#include "Utils.h"
#include "I2C_HIH_Measurement.h"
#include <linux/i2c.h>
#include "ConsumaValoriSensori.h"
#include "TaratoreValoriPolveri.h"
#include "hidapi-0.7.0/hidapi/hidapi.h"
#include "UsbUtils.h"
#include "ReadI2C.h"

using namespace tbb ;
using namespace Raspberry_Gruppo_1 ;

std::atomic<double> dExternalDustCurrentOffset ;
std::atomic<double> dExternalDustCurrentMinValue ;
concurrent_queue<double> externalDustMeasurements ;
concurrent_queue<t_HumidityAndTemperatureMeasurementData> internalHumidityAndTemperatureMeasurements ;
concurrent_queue<t_HumidityAndTemperatureMeasurementData> externalHumidityAndTemperatureMeasurements ;

// Flag che indica se il programma puo' continuare l'esecuzione oppure se si deve uscire
std::atomic<bool> bContinueExecution ;


// Callback a fronte di un signal (interrupt software) del S.O. (ad esempio CTRL+C)
// Per permettere di interrompere l'esecuzione dei cicli infiniti dei thread
// e di conseguenza terminare il programma normalmente, richiamando tutti i distruttori
void Signal_Callback_Handler( int nSignalNumber )
{
    if ( nSignalNumber == SIGINT )
    {
        bContinueExecution = false ;
    }
}

int main()
{
    try
    {
        dExternalDustCurrentOffset = DUST_OFFSET_INIT_VALUE ;
        dExternalDustCurrentMinValue = DUST_MIN_INIT_VALUE ;
        bContinueExecution = true ;

        // Registriamo il segnale interrupt Software del S.O. scatenato con il CTRL+C  e l'handler da evocare a fronte del suo
        // "lancio" da parte del S.O. stesso
        signal( SIGINT , Signal_Callback_Handler ) ;

        // Crea il thread che "consuma" i valori rilevati dai sensori
        thread threadConsumatore( ( CConsumaValoriSensori() ) ) ;
        // Detach perche' la sua esecuzione puo' viaggiare indipendente e noi non dobbiamo conoscerne lo stato
        threadConsumatore.detach() ;

        // Crea il thread per la lettura del sensore I2C interno
        // async per poterne facilmente attendere la chiusura prima del return del main, se si esce con CTRL+C
        future<void> threadI2C = async( launch::async , CReadI2C() ) ;

        // Crea il thread che "tara" i valori rilevati dal sensore delle polveri
        thread threadTaraPolveri( ( CTaratoreValoriPolveri() ) ) ;
        // Detach perche' la sua esecuzione puo' viaggiare indipendente e noi non dobbiamo conoscerne lo stato
        threadTaraPolveri.detach() ;

        CUsbUtils usbUtils ;
        unsigned char pReadBuffer[6] = { 0 , 0 , 0 , 0 , 0 , 0 } ;

        uint16_t nsReadCounter = 0 ;
        /*
        * Misuriamo le polveri ogni 10 ms e temperatura ed umidita' ogni 1000 ms
        */
        while ( bContinueExecution )
        {
            try
            {
                nsReadCounter++ ;

                int nReadedBytes = 0 ;

                if ( usbUtils.Read( pReadBuffer , 6 , &nReadedBytes ) )
                {
                    // Elaborazione della lettura delle polveri
                    uint16_t nsDust = ( pReadBuffer[0] << 8 ) + pReadBuffer[1] ;

                    externalDustMeasurements.push( CUtils::FormatDustValue( nsDust ) ) ;

                    // Se abbiamo raggiunto il valore HUMIDITY_AND_TEMPERATURE_PERIOD_COUNTER del contatore, significa che siamo pronti per
                    // processare anche temperatura ed umidita', oltre alla polvere
		    if ( nsReadCounter == HUMIDITY_AND_TEMPERATURE_PERIOD_COUNTER )
                    {
                        // Elaborazione della lettura della temperatura e dell'umidita' (esterne)
                        uint16_t nsHumidity = ( pReadBuffer[2] << 8 ) + pReadBuffer[3] ;
                        uint16_t nsTemperature = ( pReadBuffer[4] << 8 ) + pReadBuffer[5] ;
					
                        if ( nsHumidity == HUMIDITY_AND_TEMPERATURE_ERR_CODE || nsTemperature == HUMIDITY_AND_TEMPERATURE_ERR_CODE )
                        {
                            CUtils::AddWarningMessage( "Errore nel Sensore di Temperatura e Umidita' del PIC" ) ; 
                        }
                        else 
                        {
                            double dExternalTemperatureCelsius = CUtils::FormatTemperatureValue( nsTemperature ) ;
                            double dExternalHumidityPercentage = CUtils::FormatHumidityValue( nsHumidity ) ;
                            //cout << "Temperatura: " << dExternalTemperatureCelsius ;
                            //cout << " Umidita': " << dExternalHumidityPercentage << endl ;

                            // Aggiungo i valori di temperatura ed umidita'
                            t_HumidityAndTemperatureMeasurementData humidityAndTemperatureMeasurementData ;
                            humidityAndTemperatureMeasurementData.m_dHumidityPercentage = dExternalHumidityPercentage ;
                            humidityAndTemperatureMeasurementData.m_dTemperatureCelsius = dExternalTemperatureCelsius ;
                            externalHumidityAndTemperatureMeasurements.push( humidityAndTemperatureMeasurementData ) ;
                        }

                        nsReadCounter = 0 ;
                    }
                }
                else
                {
                    CUtils::AddWarningMessage( string( "main - usbUtils.Read fallita. VID Device: " ) + to_string( PIC_VID ) + string( " PID Device: " ) + to_string( PIC_PID ) ) ;
                }

                std::chrono::milliseconds sleep_duration( DUST_MEASUREMENT_PERIOD_MS ) ;
                std::this_thread::sleep_for( sleep_duration ) ;
            }
            catch( const exception& e )
            {
                CUtils::AddWarningMessage( "main - Eccezione durante la lettura dei dati dai sensori. Testo: " + string( e.what() ) ) ;
            }
        }

        threadI2C.wait() ;
    }
    catch( const exception& e )
    {
        CUtils::AddWarningMessage( "main - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }

    return 0 ;
}
