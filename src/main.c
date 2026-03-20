/*******************************************************************************
** Copyright 2025-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Source file containing the main loop, a basic user interface,
** as well as basic timer and serial port handling.
********************************************************************************
*/

/*
** For "timeGetTime()"
*/
#pragma comment( lib, "Winmm.lib" )

#include "windows.h"
#include "stdio.h"
#include "conio.h"

#include "abcc.h"
#include "abcc_log.h"
#include "abcc_hardware_abstraction.h"
#include "abcc_types.h"
#include "abcc_api.h"

extern void TP_Shutdown( void );
extern void TP_vSetPathId( UINT32 lValue );

#define DOW_FILE_WRITE_TEST (TRUE)

#if DOW_FILE_WRITE_TEST
#include "abp_fsi.h"
#define ABCC_PORT_DebugPrint( args ) printf args
ABCC_ErrorCodeType APPL_Trigger_FwUpdate( void );
#endif // DOW_FILE_WRITE_TEST


/*------------------------------------------------------------------------------
** RunUi()
** This function handles the user interface.
** Returns TRUE is the user have pressed the Q key.
**------------------------------------------------------------------------------
** Inputs:
**    -
**
** Outputs:
**    Returns:    - TRUE if user have pressed the "Q" key.
**
** Usage:
**    fQuit = RunUi();
**------------------------------------------------------------------------------
*/
BOOL8 RunUi( void )
{
   static char   abUserInput;
   BOOL8         fKbInput = FALSE;

   if( _kbhit() )
   {
      abUserInput = (char)_getch();
      fKbInput = TRUE;

      if( ( abUserInput == 'q' ) ||
          ( abUserInput == 'Q' ) )
      {
         /*
         ** Q is for quit.
         */
         return( TRUE );
      }
   #if DOW_FILE_WRITE_TEST
      else if( ( abUserInput == 't' ) ||
               ( abUserInput == 'T' ) )
      {
         APPL_Trigger_FwUpdate();
      }
   #endif // DOW_FILE_WRITE_TEST

   }
   return( FALSE );
} /* End of RunUi() */

/*------------------------------------------------------------------------------
** ABCC_API_CbfUserInit()
** Place to take action based on ABCC module network type and firmware version.
** Calls ABCC_API_UserInitComplete() to indicate to the abcc-api to continue.
**------------------------------------------------------------------------------
*/
void ABCC_API_CbfUserInit( ABCC_API_NetworkType iNetworkType, ABCC_API_FwVersionType iFirmwareVersion )
{
   printf( "\nABCC_API_CbfUserInit() entered.\n" );
   printf( " - Network type:     0x%X\n", iNetworkType );
   printf( " - Firmware version: %u.%u.%u\n", iFirmwareVersion.bMajor, iFirmwareVersion.bMinor, iFirmwareVersion.bBuild );
   printf( "Now calling ABCC_API_UserInitComplete() to progress from SETUP state to NW_INIT.\n\n" );
   ABCC_API_UserInitComplete();
   return;
}

/*------------------------------------------------------------------------------
** printReadableTime()
** Formats time from milliseconds to hours, minutes, seconds, milliseconds and
** prints it.
**------------------------------------------------------------------------------
*/
void printReadableTime( DWORD ms ) {
    uint32_t hours = ms / 3600000;  // 1 hour = 3600000 ms
    ms %= 3600000;
    uint32_t minutes = ms / 60000;  // 1 min = 60000 ms
    ms %= 60000;
    uint32_t seconds = ms / 1000;   // 1 sec = 1000 ms
    uint32_t milliseconds = ms % 1000;

    printf("%02uh %02um %02u.%03us", hours, minutes, seconds, milliseconds);
}

/*------------------------------------------------------------------------------
** main()
** Initializes the driver and runs the main loop.
**------------------------------------------------------------------------------
*/
int main( void )
{
   /*
   ** Note: Make sure the ABCC reset signal is kept low by the platform specific
   ** initialization to keep the ABCC module in reset until the driver releases
   ** it.
   */

   BOOL8          fQuit = FALSE;
   ABCC_ErrorCodeType eErrorCode = ABCC_EC_NO_ERROR;
   const UINT16   iSleepTimeMS = 10;
   DWORD          lThen, lNow, lDiff;

#if( _DEBUG )
   /*
   ** Set a hard-coded Path ID if this is a debug build. In those cases we
   ** probably want to use the same, and already known, Path ID for every run.
   ** Since possible Path IDs start at '1' the value '0' means "let the user
   ** select the path".
   **
   ** For more information about Microsoft's predefined macros see
   ** https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
   */
   TP_vSetPathId( 0 );
#endif


   printf( "-------------------------------------------------\n" );
   printf( "Program started at: " );
   printReadableTime( timeGetTime() );
   printf( "\n" );
   printf( "-------------------------------------------------\n" );
   printf( "HMS Networks\n" );
   printf( "Anybus CompactCom Starter Kit\n" );
   printf( "Windows example port\n\n" );
   printf( "Press 'Q' to quit.\n\n" );

   /*
   ** Function to initialize CompactCom-related systems.
   ** Note: This function in not required to call unless
   ** ABCC_HAL_HwInit() contain anything.
   */
   if( ABCC_API_Init() != ABCC_EC_NO_ERROR )
   {
      return( 0 );
   }

   lThen = timeGetTime();

   while( !fQuit  )
   {
      /*
      ** Primary function start and drive the abcc-api.
      */
      eErrorCode = ABCC_API_Run();
      /*
      ** Handle potential error codes returned from the abcc-api here.
      */
      if( eErrorCode != ABCC_EC_NO_ERROR )
      {
         printf( "ABCC_API_Run() returned status code: %d\n", eErrorCode );
         fQuit = TRUE;
      }
      else
      {
         fQuit = RunUi();
      }

      lNow = timeGetTime();
      lDiff = lNow - lThen;
      if( lDiff > 0 )
      {
         /*
         ** Truncate intervals to 65535ms because 'ABCC_API_RunTimerSystem()'
         ** takes a UINT16.
         */
         if( lDiff > (UINT16)0xFFFF )
         {
            lDiff = (UINT16)0xFFFF;
         }

         /*
         ** Provide the abcc-api with a time base. Required for timers to function.
         */
         ABCC_API_RunTimerSystem( (UINT16)lDiff );
         lThen = lNow;
      }

      if( iSleepTimeMS > 0 )
      {
         Sleep( iSleepTimeMS );
      }
   }

   /*
   ** Shut down the abcc-api and the CompactCom.
   */
   ABCC_API_Shutdown();

   TP_Shutdown();

   if( eErrorCode != ABCC_EC_NO_ERROR )
   {
      printf( "Press any key to quit.\n" );
      while( !_kbhit() )
      {
         Sleep( 100 );
      }
   }

   return ( 0 );

} /* End of main() */

#if DOW_FILE_WRITE_TEST

#define DOW_MAX_FILE_SIZE_BYTES ( 5 * 1024 * 1024 )
INT16 appl_iFragSize = ABCC_CFG_SPI_DEFAULT_MSG_FRAG_LEN;
BOOL appl_fFileWriteOngoing = FALSE;
UINT16 appl_iFwUpdateInstance = 0xFFFF;
UINT16 appl_iWrittenSize;
UINT32 appl_lTotalFileSizeCounter = 0, appl_lTotalFileSize = 0;
UINT8 appl_abDummyData[ 1524 ];

void FwUpdateDeleteCallback( UINT16 iInstance, ABP_MsgErrorCodeType eMsgResult, UINT8 bFsiError )
{
   if( eMsgResult == ABP_ERR_NO_ERROR )
   {
      ABCC_PORT_DebugPrint( ( "Process completed\n" ) );

      appl_iFragSize = ABCC_CFG_SPI_DEFAULT_MSG_FRAG_LEN;
      ABCC_NewMsgFragSize( appl_iFragSize );
      appl_iFwUpdateInstance = 0xFFFF;
      appl_fFileWriteOngoing = FALSE;
   }
   else
   {
      ABCC_PORT_DebugPrint( ( "FwUpdateDeleteCallback Error: %i, %i\n", eMsgResult, bFsiError ) );
   }
}

void FwUpdateFileCloseCallback( UINT16 iInstance, ABP_MsgErrorCodeType eMsgResult, UINT8 bFsiError )
{
   if( eMsgResult == ABP_ERR_NO_ERROR )
   {
      // appl_iTotalFileSize is now valid and can be verified ...
      ABCC_PORT_DebugPrint( ( "Total File Size:          %i \n", appl_lTotalFileSize ) );
      ABCC_PORT_DebugPrint( ( "Calculated File Size was: %i \n", appl_lTotalFileSizeCounter ) );
      appl_lTotalFileSize = 0;
      appl_lTotalFileSizeCounter = 0;
      ABCC_PORT_DebugPrint( ( "ANB_FSI_Delete() Result: %i \n",
         ANB_FSI_Delete( appl_iFwUpdateInstance, FwUpdateDeleteCallback ) ) );
   }
   else
   {
      ABCC_PORT_DebugPrint( ( "FwUpdateFileCloseCallback  Error: %i, %i\n", eMsgResult, bFsiError ) );
   }
}

void FwUpdateFileWriteCallback( UINT16 iInstance, ABP_MsgErrorCodeType eMsgResult, UINT8 bFsiError )
{
   static BOOL fCountUp = TRUE;

   if( eMsgResult == ABP_ERR_NO_ERROR )
   {
      // appl_iWrittenSize is now valid and should eb used to increase the offset within the data to be written...
      ABCC_PORT_DebugPrint( ( "Number of written bytes: %i:\n", appl_iWrittenSize ) );
      appl_lTotalFileSizeCounter += appl_iWrittenSize;
      ABCC_PORT_DebugPrint( ( "Calculated File Size: %i:\n", appl_lTotalFileSizeCounter) );

      if( appl_lTotalFileSizeCounter >= DOW_MAX_FILE_SIZE_BYTES )
      {
         // max defined file size reached
         // => prepare next call
         fCountUp = TRUE;
         //    and close file
         ABCC_PORT_DebugPrint( ( "Maximum defined file size reached => Close file\n" ) );
         ABCC_PORT_DebugPrint( ( "ANB_FSI_FileClose() Result: %i:\n",
            ANB_FSI_FileClose( appl_iFwUpdateInstance, &appl_lTotalFileSize, FwUpdateFileCloseCallback ) ) );
      }
      else if( appl_iFragSize <= ABCC_CFG_SPI_MIN_MSG_FRAG_LEN )
      {
         // minimum fragment size reached
         // => prepare next call
         fCountUp = TRUE;
         //    and close file
         ABCC_PORT_DebugPrint( ( "Smallest Fragment successfully tested => Close file\n" ) );
         ABCC_PORT_DebugPrint( ( "ANB_FSI_FileClose() Result: %i:\n",
            ANB_FSI_FileClose( appl_iFwUpdateInstance, &appl_lTotalFileSize, FwUpdateFileCloseCallback ) ) );
      }
      else
      {
         if( appl_iFragSize >= ABCC_CFG_SPI_MAX_MSG_FRAG_LEN )
         {
            ABCC_PORT_DebugPrint( ( "Biggest allowed Fragment successfully tested => start decreasing Message Fragment size\n" ) );
            fCountUp = FALSE;
         }
         if( fCountUp )
         {
            appl_iFragSize += ( timeGetTime() % 32 );
            if( appl_iFragSize > ABCC_CFG_SPI_MAX_MSG_FRAG_LEN )
            {
               appl_iFragSize = ABCC_CFG_SPI_MAX_MSG_FRAG_LEN;
            }
         }
         else
         {
            appl_iFragSize -= ( timeGetTime() % 32 );
            if( appl_iFragSize < ABCC_CFG_SPI_MIN_MSG_FRAG_LEN )
            {
               appl_iFragSize = ABCC_CFG_SPI_MIN_MSG_FRAG_LEN;
            }
         }

         ABCC_API_NewMsgFragSize( appl_iFragSize );
         //EXTFUNC ABCC_ErrorCodeType ANB_FSI_FileWrite( UINT16 iInstance, UINT8* pbSrc, UINT16 iReqSize, UINT16* piActSize, ANB_FSI_CompletionCbfType pnCallback );
         ABCC_PORT_DebugPrint( ( "ANB_FSI_FileWrite() Result: %i:\n",
            ANB_FSI_FileWrite( appl_iFwUpdateInstance, appl_abDummyData, 1524, &appl_iWrittenSize, FwUpdateFileWriteCallback ) ) );
      }
   }
   else
   {
      ABCC_PORT_DebugPrint( ( "FwUpdateFileWriteCallback Error: %i, %i\n", eMsgResult, bFsiError ) );
   }
}

void FwUpdateFileOpenCallback( UINT16 iInstance, ABP_MsgErrorCodeType eMsgResult, UINT8 bFsiError )
{
   if( eMsgResult == ABP_ERR_NO_ERROR )
   {
      ABCC_PORT_DebugPrint( ( "File Opened\n" ) );
      //EXTFUNC ABCC_ErrorCodeType ANB_FSI_FileWrite( UINT16 iInstance, UINT8* pbSrc, UINT16 iReqSize, UINT16* piActSize, ANB_FSI_CompletionCbfType pnCallback );
      ABCC_PORT_DebugPrint( ( "ANB_FSI_FileWrite() Result: %i:\n",
         ANB_FSI_FileWrite( appl_iFwUpdateInstance, appl_abDummyData, 1524, &appl_iWrittenSize, FwUpdateFileWriteCallback ) ) );
   }
   else
   {
      ABCC_PORT_DebugPrint( ( "FwUpdateFileOpenCallback Error: %i, %i\n", eMsgResult, bFsiError ) );
   }
}

void FwUpdateCreateCallback( UINT16 iInstance, ABP_MsgErrorCodeType eMsgResult, UINT8 bFsiError )
{
   if( eMsgResult == ABP_ERR_NO_ERROR )
   {
      ABCC_PORT_DebugPrint( ( "instance Created: %i:\n", iInstance ) );

      appl_iFwUpdateInstance = iInstance;
      //EXTFUNC ABCC_ErrorCodeType ANB_FSI_FileOpen( UINT16 iInstance, char* pacName, UINT8 bMode, ANB_FSI_CompletionCbfType pnCallback );
      ABCC_PORT_DebugPrint( ( "ANB_FSI_FileOpen() Result: %i:\n",
         ANB_FSI_FileOpen( appl_iFwUpdateInstance, "\\test.txt", ABP_FSI_FILE_OPEN_WRITE_MODE, FwUpdateFileOpenCallback ) ) );
      for( UINT16 i = 0; i < 1524; i++ )
      {
         switch( i )
         {
         case 0:
            appl_abDummyData[ i ] = 'D';
            break;
         case 1:
            appl_abDummyData[ i ] = 'u';
            break;
         case 2:
         case 3:
            appl_abDummyData[ i ] = 'm';
            break;
         case 4:
            appl_abDummyData[ i ] = 'y';
            break;
         case 5:
            appl_abDummyData[ i ] = 'D';
            break;
         case 6:
            appl_abDummyData[ i ] = 'a';
            break;
         case 7:
            appl_abDummyData[ i ] = 't';
            break;
         case 8:
            appl_abDummyData[ i ] = 'a';
            break;
         default:
            appl_abDummyData[ i ] = 0x30 + i % 10;
            break;
         }
      }
   }
   else
   {
      ABCC_PORT_DebugPrint( ( "FwUpdateCreateCallback Error: %i, %i\n", eMsgResult, bFsiError ) );
   }
}

ABCC_ErrorCodeType APPL_Trigger_FwUpdate( void )
{
   static BOOL fInitialized = FALSE;
   ABCC_ErrorCodeType eResult = ABCC_EC_NO_ERROR;
   if( !appl_fFileWriteOngoing )
   {
      if( fInitialized == FALSE )
      {
         ANB_FSI_Init();
         fInitialized = TRUE;
      }
      eResult = ANB_FSI_Create( FwUpdateCreateCallback );
      ABCC_PORT_DebugPrint( ( "ANB_FSI_Create() Result: %i:\n", eResult ) );
      if( eResult == ABCC_EC_NO_ERROR );
      {
         appl_fFileWriteOngoing = TRUE;
      }
      return eResult;
   }
   else
   {
      return ABCC_EC_NO_RESOURCES;
   }
}
#endif // DOW_DYNAMIC_SPI_MSG_FRAG_LEN

