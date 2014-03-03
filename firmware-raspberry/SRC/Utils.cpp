#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <limits>
#include <thread>
#include <mutex>
#include <memory>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std ;

#include "MeasurementData.h"
#include "tbb/tbb.h"
#include "tbb/concurrent_queue.h"
#include "LibcurlUtils.h"

#include "Utils.h"

using namespace tbb ;
using namespace Raspberry_Gruppo_1 ;

extern std::atomic<double> dExternalDustCurrentOffset ; 
extern std::atomic<double> dExternalDustCurrentMinValue ;
extern concurrent_queue<t_MeasurementDataToResend> valuesToResend ;


vector<string> CUtils::m_arsWarningMessages ;
std::mutex CUtils::m_Mutex ;

string CUtils::GetCurrentDatetimeISO8601Formatted()
{
    string sRet = "" ;
    try
    {
        // Si ricava l'ora corrente
        timeval curTimeval ;
        gettimeofday( &curTimeval , NULL ) ;
        struct tm* lt = localtime( &curTimeval.tv_sec ) ;
        // Creazione della prima parte della data, senza millisecondi e timezone
        unique_ptr<char[]> pUniquePtrCurrentTime( new char[ 20 ] ) ;
        strftime( pUniquePtrCurrentTime.get() , 20 , "%FT%T", lt ) ;
        unique_ptr<char[]> pUniquePtrCurrentTimeMilliseconds( new char[ 4 ] ) ;
        sprintf( pUniquePtrCurrentTimeMilliseconds.get() , ".%03d" , (uint32_t)( curTimeval.tv_usec / 1000 ) ) ;
        // Creazione della parte sul timezone
        unique_ptr<char[]> pUniquePtrTimezone( new char[ 6 ] ) ;
        strftime( pUniquePtrTimezone.get() , 20 , "%z", lt ) ;
        // Concatenazione delle  due parti appena creazione, con i millisecondi in mezzo
        sRet = string( pUniquePtrCurrentTime.get() ) + pUniquePtrCurrentTimeMilliseconds.get() + string( pUniquePtrTimezone.get() ) ;
    }
    catch( const exception& e )
    {
        AddWarningMessage( "CUtils::GetCurrentDatetimeISO - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }

    return sRet ;
}

bool CUtils::ReadUnsendedMeasurementsFromFile()
{
    bool bRet = false ;

    try
    {
        struct flock fl ;

        fl.l_type   = F_RDLCK ;
        fl.l_whence = SEEK_SET ;
        fl.l_start  = 0 ;        // Offset da l_whence
        fl.l_len    = 0 ;        // length 0 = fino a EOF
        fl.l_pid    = getpid() ; // Il nostro PID

        int nFd = -1 ;
        // Necessaria per poter lock-are il file
        // O_RDONLY: Apre un file per la lettura.
        if ( ( nFd = open( MEASUREMENTS_TO_SEND_FILE_NAME , O_RDONLY ) ) == -1 )
        {
            cout << "Impossibile aprire il file di backup delle misure." << endl ;
        }
        else
        {
            // Sfruttiamo il file descriptor per ottenere anche la struttura FILE* e facilitarci la vita con le funzioni di lettura
            FILE* pFilePointer = NULL ;
            if ( ( pFilePointer = fdopen( nFd , "r" ) ) == NULL )
            {
                AddWarningMessage( "CUtils::ReadUnsendedMeasurementsFromFile - Impossibile aprire il file di backup delle misure usando il file descriptor assegnato dal sistema operativo." ) ;
            }
            else
            {
                // Lock. Si rimane bloccati finche' la richiesta non puo' essere esaudita
                if ( fcntl( nFd , F_SETLKW , &fl ) == -1 )
                {
                    AddWarningMessage( "CUtils::ReadUnsendedMeasurementsFromFile - Acquisizione del lock sul file di backup delle misure fallita!" , true ) ;
                }
                else
                {
                    char* arcLine = NULL ;
                    ssize_t nLineSize ;
                    size_t nLen = 0 ;
                    // Lettura, linea per linea, del file
                    while ( ( nLineSize = getline( &arcLine , &nLen , pFilePointer ) ) != -1 )
                    {
                        int nRetryCount = 0 ;
                        unique_ptr<char[]> pUniquePtrChars( new char[ strlen( arcLine ) - 1 ] ) ;
                        sscanf( arcLine , "%s %d" , pUniquePtrChars.get() , &nRetryCount ) ;

                        t_MeasurementDataToResend measurementDataToResend ;
                        measurementDataToResend.m_sMeasurementDataToResend = pUniquePtrChars.get() ;
                        measurementDataToResend.m_nRetryCount = nRetryCount ;

                        // Inserimento nella coda delle misure da rimandare
                        valuesToResend.push( measurementDataToResend ) ;
                    }

                    // Unlock del file
                    fl.l_type = F_UNLCK ;
                    if ( fcntl( nFd , F_SETLK , &fl ) == -1 )
                    {
                        AddWarningMessage( "CUtils::ReadUnsendedMeasurementsFromFile - Rilascio del lock sul file di backup delle misure fallita!" , true ) ;
                    }
                    else
                    {
                        bRet = true ;
                    }
                }
                
                fclose( pFilePointer ) ;
            }
        }
    }
    catch( const exception& e )
    {
        AddWarningMessage( "CUtils::ReadUnsendedMeasurementsFromFile - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }

    return bRet ;
}

bool CUtils::WriteUnsendedMeasurementsToFile( string sFilePath , string& sUnsendedMeasurementData , int nRetryCount )
{
    bool bRet = false ;

    try
    {
        struct flock fl ;

        fl.l_type   = F_WRLCK ;
        fl.l_whence = SEEK_SET ;
        fl.l_start  = 0 ;        // Offset da l_whence
        fl.l_len    = 0 ;        // length 0 = fino a EOF
        fl.l_pid    = getpid() ; // Il nostro PID
        int nFd     = -1 ;

        // Necessaria per poter lock-are il file
        // O_WRONLY: Apre un file per la scrittura con permessi di lettura e scrittura per chiunque. La posizione e' all'inizio del file
        // O_CREAT: Se pathname non esiste viene creato. In mancanza di questo flag, viene tornato un errore.
        // O_APPEND: Apre il file in append. Le successive operazioni di scrittura verranno accodate alle informazioni gia' presenti nel file stesso.
        if ( ( nFd = open( sFilePath.c_str() , O_WRONLY|O_CREAT|O_APPEND , 0666 ) ) == -1 )
        {
            AddWarningMessage( "CUtils::WriteUnsendedMeasurementsToFile - Impossibile aprire il file di backup delle misure." ) ;
        }
        else
        {
            // Sfruttiamo il file descriptor per ottenere anche la struttura FILE* e facilitarci la vita con le funzioni di scrittura
            FILE* pFilePointer = NULL ;
            if ( ( pFilePointer = fdopen( nFd , "w" ) ) == NULL )
            {
                close( nFd ) ;
                AddWarningMessage( "CUtils::WriteUnsendedMeasurementsToFile - Impossibile aprire il file di backup delle misure usando il file descriptor assegnato dal sistema operativo." ) ;
            }
            else
            {
                // Lock. Si rimane bloccati finche' la richiesta non puo' essere esaudita
                if ( fcntl( nFd , F_SETLKW , &fl ) == -1 )
                {
                    AddWarningMessage( "CUtils::WriteUnsendedMeasurementsToFile - Acquisizione del lock sul file di backup delle misure fallita!" , true ) ;
                }
                else
                {
                    // Scrittura sul file. Formato: misura retry
                    fprintf( pFilePointer , "%s %d\n" , sUnsendedMeasurementData.c_str() , nRetryCount ) ;

                    // Unlock del file
                    fl.l_type = F_UNLCK ;
                    if ( fcntl( nFd , F_SETLK , &fl ) == -1 )
                    {
                        AddWarningMessage( "CUtils::WriteUnsendedMeasurementsToFile - Rilascio del lock sul file di backup delle misure fallita!" , true ) ;
                    }
                    else
                    {
                        bRet = true ;
                    }
                }

                fclose( pFilePointer ) ;
            }
        }
    }
    catch( const exception& e )
    {
        AddWarningMessage( "CUtils::WriteUnsendedMeasurementsToFile - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }

    return bRet ;
}

bool CUtils::ClearUnsendedMeasurementsFile()
{
    bool bRet = false ;

    try
    {
        struct flock fl ;

        fl.l_type   = F_WRLCK ;
        fl.l_whence = SEEK_SET ;
        fl.l_start  = 0 ;        // Offset da l_whence
        fl.l_len    = 0 ;        // length 0 = fino a EOF
        fl.l_pid    = getpid() ; // Il nostro PID

        int nFd = -1 ;

        // Necessaria per poter lock-are il file
        // O_WRONLY: Apre un file per la scrittura. La posizione e' all'inizio del file
        if ( ( nFd = open( MEASUREMENTS_TO_SEND_FILE_NAME , O_WRONLY ) ) == -1 )
        {
            cout << "Impossibile aprire il file di backup delle misure." << endl ;
        }
        else
        {
            // Lock. Si rimane bloccati finche' la richiesta non puo' essere esaudita
            if ( fcntl( nFd , F_SETLKW , &fl ) == -1 )
            {
                AddWarningMessage( "CUtils::ClearUnsendedMeasurementsFile - Acquisizione del lock sul file di backup delle misure fallita!" , true ) ;
            }
            else
            {
                // Svuotiamo il file
                ftruncate( nFd , 0 ) ;

                // Unlock del file
                fl.l_type = F_UNLCK ;
                if ( fcntl( nFd , F_SETLK , &fl ) == -1 )
                {
                    AddWarningMessage( "CUtils::ClearUnsendedMeasurementsFile - Rilascio del lock sul file di backup delle misure fallita!" , true ) ;
                }
                else
                {
                    bRet = true ;
                }
            }

            close( nFd ) ;
        }
    }
    catch( const exception& e )
    {
        AddWarningMessage( "CUtils::ClearUnsendedMeasurementsFile - Eccezione generica. Testo: " + string( e.what() ) ) ;
    }

    return bRet ;
}

double CUtils::FormatDustValue( uint16_t nsDustTicks )
{
    /*	
    * Per il range di tensione che va da 0 a 3.3 V, abbiamo valori di nsDustTicks espressi con un intero che varia tra 0 e 1023.
    * Per prima cosa quindi, convertiamo il valore intero misurato, nella tensione corrispondente.
    */
    double dDustSensorVoltage =  nsDustTicks * 4.0 / 1024.0 ;
    /*
    * Adesso occorre convertire il voltaggio appena ricavato, mediante l'equazione lineare fornita dalle specifiche in nostro
    * possesso, nel valore finale espresso in microgrammi al metro cubo.
    */
    double dDustValue = ( ( 0.172 * dDustSensorVoltage ) - 0.0999 ) * 1000.0 ;
    // Se il valore e' minore del minimo attuale, allora diventa egli stesso il valore minimo
    if ( dDustValue < dExternalDustCurrentMinValue )
    {
        dExternalDustCurrentMinValue = dDustValue ;
    }

    return dDustValue ;
}

double CUtils::FormatHumidityValue( uint16_t nsHumidityTicks )
{
    // Clear dei bit di stato
    /*
    * dalla sezione 4.0 del data sheet
    * Formula RH = ( ticks / ( 2^14 -2 ) ) * 100
    */
    return ( ( nsHumidityTicks & 0x3FFF ) / 16382.0 ) * 100.0 ;
}

double CUtils::FormatTemperatureValue( uint16_t nsTemperatureTicks )
{
    // Shift a destra per eliminare i bit inutilizzati   
    /*
    * dalla sezione 5.0 del data sheet
    * Formula T = ( ticks / ( 2^14 - 2 ) ) * 165 -40
    */
    return ( ( nsTemperatureTicks >> 2 ) / 16382.0 ) * 165.0 - 40.0 ;
}

bool CUtils::AddWarningMessage( string sWarningMessage )
{
    return AddWarningMessage( sWarningMessage , false ) ;
}

bool CUtils::AddWarningMessage( string sWarningMessage , bool bSendEmail )
{
    bool bRet = false ;

    try
    {
        // Lock per evitare che arrivi un altro messaggio proprio mentre stiamo per inviare l'email,
        // cosa che provocherebbe l'invio di un'email doppia ( oltre a rendere non thread-safe il vettore
        // dei messaggi, poiche' potremmo avere potenzialmente piu' thread che lo utilizzano nello stesso
        // momento. Con questo Lock, il vettore non corre piu' questo rischio, poiche' la sequenza di
        // operazioni di Add, Get e Clear e' comunque gestita tutta in questa routine.
        // Se le cose dovessero cambiare, aggiungendo altri punti dove si puo' accedere e manipolare il
        // vettore, occorre riprogettare la logica di Lock opportunamente
        std::lock_guard<std::mutex> lockGuard( m_Mutex ) ;

        sWarningMessage = CUtils::GetCurrentDatetimeISO8601Formatted() + " - " + sWarningMessage + "\n" ;

        cout << sWarningMessage << endl ;

        m_arsWarningMessages.push_back( sWarningMessage ) ;

        // Se ho raggiunto il numero di messaggi sufficienti per un invio dell'email oppure devo inviare in ogni caso l'email, procedo
        if ( bSendEmail || m_arsWarningMessages.size() >= MAX_QUEUED_MESSAGES_BEFORE_EMAIL )
        {
            // Se ho in coda troppi messaggi ( probabilmente a causa del fallimento dell'invio dell'email )
            // non invio la mail e "pulisco" il vector, onde evitare un OutOfMemory
            if ( m_arsWarningMessages.size() >= MAX_QUEUED_MESSAGES_CAUSED_BY_ERRORS )
            {
                m_arsWarningMessages.clear() ;
                m_arsWarningMessages.shrink_to_fit() ;
            }
            else
            {
                // Invio dell'email
                CLibcurlUtils::SendEmailReport() ;
            }
        }

        bRet = true ;
    }
    catch( const exception& e )
    {
        cout << "CUtils::AddWarningMessage - WARNING - Eccezione generica. Testo: " << e.what() << endl ;
    }

    return bRet ;
}

void CUtils::ClearWarningMessages()
{
    m_arsWarningMessages.clear() ;
}
