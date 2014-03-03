#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <limits>

#include "MeasurementData.h"

using namespace std ;

#include "Utils.h"

#include "TaratoreValoriPolveri.h"

using namespace Raspberry_Gruppo_1 ;

extern atomic<double> dExternalDustCurrentOffset ; 
extern atomic<double> dExternalDustCurrentMinValue ; 
extern atomic<bool> bContinueExecution ;


void CTaratoreValoriPolveri::operator()()
{
    while( bContinueExecution )
    {
        // Calcola il minimo dei valori ogni giorno
        chrono::milliseconds sleep_duration( DUST_MIN_VALUE_PERIOD_MS ) ;
        this_thread::sleep_for( sleep_duration ) ;

        CalculateDustOffset() ;
    }
}

void CTaratoreValoriPolveri::CalculateDustOffset()
{
    try
    {
        // L'offset e' il valore minimo rilevato in questo intervallo di tempo, cambiato di segno
        dExternalDustCurrentOffset = -dExternalDustCurrentMinValue ;

        // Ripristino il valore iniziale del minimo, in modo che il prossimo calcolo dell'offset tenga conto solo dei valori
        // rilevati da adesso fino al prossimo calcolo dell'offset
        dExternalDustCurrentMinValue = DUST_MIN_INIT_VALUE ;
    }
    catch( const exception& e )
    {
        CUtils::AddWarningMessage( "CTaratoreValoriPolveri::CalculateDustOffset - Eccezione durante il calcolo dell'offset. Testo eccezione: " + string( e.what() ) + "dExternalDustCurrentMinValue: " + to_string( dExternalDustCurrentMinValue ) ) ;
    }
}
