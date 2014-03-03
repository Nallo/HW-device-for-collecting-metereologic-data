#include <iostream>
#include <stdlib.h>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include "hidapi-0.7.0/hidapi/hidapi.h"

using namespace std ;

#include "Utils.h"

#include "UsbUtils.h"

using namespace Raspberry_Gruppo_1 ;


CUsbUtils::CUsbUtils()
{
    Init() ;
}

CUsbUtils::~CUsbUtils()
{
    try
    {
        // Se il Device e' aperto, lo chiude
        if ( m_pDeviceHandle != NULL )
        {
            hid_close( m_pDeviceHandle ) ;

            m_pDeviceHandle = NULL ;
        }

        // Libera gli oggetti statici di HIDAPI
        hid_exit() ;
    }
    catch( const exception& e )
    {
        CUtils::AddWarningMessage( "CUsbUtils::~CUsbUtils - Eccezione durante il rilascio delle risorse di hidapi. Testo: " + string( e.what() ) ) ;
    }
}

void CUsbUtils::Init()
{
    m_pDeviceHandle = NULL ;

    try
    {
        // Apre il dispositivo con il VID ed il PID,
        // e opzionalmente con il Serial number.
        m_pDeviceHandle = hid_open( PIC_VID , PIC_PID , NULL ) ;
        if ( !m_pDeviceHandle )
        {
            CUtils::AddWarningMessage( string( "CUsbUtils::Init - Errore durante la hid_open. VID Device: " ) + to_string( PIC_VID ) + " PID Device: " + to_string( PIC_PID ) ) ;
        }
        else
        {
            cout << "Dispositivo Aperto." << endl ;
        }
    }
    catch( const exception& e )
    {
        CUtils::AddWarningMessage( string( "CUsbUtils::Init - Eccezione durante l'apertura del Device USB. VID Device: " ) + to_string( PIC_VID ) + " PID Device: " + to_string( PIC_PID ) + " Testo: " + string( e.what() ) ) ;
    }
}

// Legge da un dispositivo USB
bool CUsbUtils::Read( unsigned char* pReadBuffer , int nSize , int* nReadedBytes )
{
    bool bRet = false ;
    
    if ( pReadBuffer != NULL && m_pDeviceHandle != NULL )
    {
        try
        {
#ifdef USE_USB_NONBLOCKING
            // Imposta la hid_read per non essere bloccante
	    hid_set_nonblocking( m_pDeviceHandle , 1 ) ;

            int nReadRetryNumber = 0 ;
	    int nRes = 0 ;
	    while ( nRes == 0 && nReadRetryNumber < USB_NONBLOCKING_RETRY_COUNT )
            {
	        nRes = hid_read( m_pDeviceHandle , pReadBuffer , nSize ) ;

                if ( nRes == 0 )
                {
                    nReadRetryNumber++ ;

                    std::chrono::milliseconds sleep_duration( USB_NONBLOCKING_NEXT_RETRY_DELAY_MS ) ;
                    std::this_thread::sleep_for( sleep_duration ) ;
                }
		else if ( nRes < 0 )
                {
                    break ;
                }
	    }
#else // USE_USB_NONBLOCKING
            int nRes = hid_read_timeout( m_pDeviceHandle , pReadBuffer , nSize , USB_READ_TIMEOUT_MS ) ;
#endif // USE_USB_NONBLOCKING
            if ( nRes > 0 )
            {
                *nReadedBytes = nRes ;
                bRet = true ;
            }
            else
            {
                *nReadedBytes = 0 ;

                wstring wsHidError( hid_error( m_pDeviceHandle ) ) ;
                CUtils::AddWarningMessage( string( "CUsbUtils::Read - Lettura fallita. Valore di ritorno: " ) + to_string( nRes ) + string( " Stringa errore: " ) + string( wsHidError.begin() , wsHidError.end() ) + string( " nSize: " ) + to_string( nSize ) ) ;
            }
        }
        catch( const exception& e )
        {
            *nReadedBytes = 0 ;

            CUtils::AddWarningMessage( "CUsbUtils::Read - Eccezione durante la lettura. Testo: " + string( e.what() ) ) ;
        }
    }

    return bRet ;
}
