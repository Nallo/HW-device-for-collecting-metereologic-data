#include <iostream>
#include <vector>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <limits>
#include "tbb/tbb.h"
#include "tbb/concurrent_queue.h"

using namespace std ;

#include "Utils.h"
#include "MeasurementData.h"

#include "I2C_HIH_Measurement.h"

using namespace tbb ;
using namespace Raspberry_Gruppo_1 ;

extern concurrent_queue<t_HumidityAndTemperatureMeasurementData> internalHumidityAndTemperatureMeasurements ;

extern std::atomic<bool> bContinueExecution ;


CI2C_HIH_Measurement::CI2C_HIH_Measurement()
{
    Init() ;
}

CI2C_HIH_Measurement::~CI2C_HIH_Measurement()
{
    if ( m_bInited )
    {
        try
        {
            if ( close( m_nI2CFileDescriptor ) < 0 )
            {
                CUtils::AddWarningMessage( string( "CI2C_HIH_Measurement::~CI2C_HIH_Measurement - Errore durante la chiusura del dispositivo I2C. Nome Device: " ) + STR_I2C_HIH_DEVICE_NAME ) ;
            }
        }
        catch ( const exception& e )
        {
            CUtils::AddWarningMessage( "CI2C_HIH_Measurement::~CI2C_HIH_Measurement - Eccezione generica. Testo: " + string( e.what() ) ) ;
        }
    }
}

void CI2C_HIH_Measurement::Init()
{
    m_bInited = false ;

    try
    {
        m_nI2CFileDescriptor = open( STR_I2C_HIH_DEVICE_NAME , O_RDWR ) ;
        if ( m_nI2CFileDescriptor < 0 )
        {
            CUtils::AddWarningMessage( string( "CI2C_HIH_Measurement::Init - Errore durante l'apertura del dispositivo I2C. Nome Device: " ) + string( STR_I2C_HIH_DEVICE_NAME ) ) ;
        }
        else
        {
            if ( ioctl( m_nI2CFileDescriptor , I2C_SLAVE , I2C_HIH_DEVICE_ID ) < 0 )
            {
                CUtils::AddWarningMessage( "CI2C_HIH_Measurement::Init - Errore durante la connessione al dispositivo come SLAVE." ) ;
            }
            else
            {
                m_bInited = true ;
            }
        }
    }
    catch ( const exception& e )
    {
        CUtils::AddWarningMessage( "CI2C_HIH_Measurement::Init - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }
}

void CI2C_HIH_Measurement::ReadInternalHumidityAndInternalTemperature()
{
    if ( m_bInited )
    {
        while ( bContinueExecution )
        {
            try
            {
                // Nel primo byte metto l'indirizzo shiftato a sinistra di 1
                // cosi come richiesto dal protocollo del nostro sensore.
                // 0x27 << 1 = 0x4E
                uint8_t pData[4] = { 0x4E , 0 , 0 , 0 } ;
                if ( write( m_nI2CFileDescriptor , pData , 1 ) != 1 )
                {
                    CUtils::AddWarningMessage( "CI2C_HIH_Measurement::ReadInternalHumidityAndInternalTemperature - Errore durante la scrittura di abilitazione alla misura sul dispositivo." ) ;
                }
                else
                {
                    // Sleep per dare il tempo al dispositivo di effettuare la misura
                    // (consigliata dal data sheet poiche' il ciclo di misura e' di circa 36.65 ms)
                    std::chrono::milliseconds sleep_duration( 40 ) ;
                    std::this_thread::sleep_for( sleep_duration ) ;

                    if ( read( m_nI2CFileDescriptor , pData , 4 ) == 4 )
                    {
                        // Controllo il 2bit per vedere se il dato ricevute' vecchio
                        // il bit e' il secondo partendo dall'MSB, peio'
                        // 0b01000000 = 0x40
                        // se != 0 dato vecchio
                        if ( pData[0] & 0x40 )
                        {
                            cout << "<<<<< I2C : Dato Vecchio >>>>>" << endl ;
                        }
                        else
                        {
                            double dInternalTemperatureCelsius = CUtils::FormatTemperatureValue( ( pData[2] << 8 ) + pData[3] ) ;
                            double dInternalHumidityPercentage = CUtils::FormatHumidityValue( (pData[0] << 8) + pData[1] ) ;
                            //cout << "----- Temperatura: " << dInternalTemperatureCelsius << " Umidita: " << dInternalHumidityPercentage << endl ;
                        
                            // Aggiungo i valori di temperatura ed umidita'
                            t_HumidityAndTemperatureMeasurementData humidityAndTemperatureMeasurementData ;
                            humidityAndTemperatureMeasurementData.m_dHumidityPercentage = dInternalHumidityPercentage ;
                            humidityAndTemperatureMeasurementData.m_dTemperatureCelsius = dInternalTemperatureCelsius ;
                            internalHumidityAndTemperatureMeasurements.push( humidityAndTemperatureMeasurementData ) ;

                           /*
                            * Mentre la misura puo' esser completata in circa 40 ms il sensore necessita
                            * di piu' tempo per reagire ai cambiamenti esterni. Quanto velocemente
                            * possa reagire dipende da fattori fuori dal nostro diretto controllo.
                            * Il datasheet specifica come maximum 'Response time' per l'umidita' a 8s
                            * e per la temperatura a 30s sotto certe condizioni specifiche.
                            * Noi leggiamo il sensore una volta al secondo.
                            * Questo compromesso fa si che si interroghi il sensore piu' velocemente di
                            * quanto possa reagire ai cambiamenti esterni, tenendo anche conto di eventuali
                            * tempi di risposti migliori dovuti a condizioni piu' favorevoli di quelli
                            * segnalati sul datasheet.
                            */

                            std::chrono::milliseconds sleep_duration( HUMIDITY_AND_TEMPERATURE_PERIOD_MS ) ;
                            std::this_thread::sleep_for( sleep_duration ) ;
                        }
                    }
                    else
                    {
                        CUtils::AddWarningMessage( "CI2C_HIH_Measurement::ReadInternalHumidityAndInternalTemperature - Errore seconda write." ) ;
                    }
                }
            }
            catch( const exception& e )
            {
                CUtils::AddWarningMessage( "CI2C_HIH_Measurement::ReadInternalHumidityAndInternalTemperature - Eccezione durante la lettura dei dati dai sensori. Testo: " + string( e.what() ) ) ;
            }
        }
    }
}
