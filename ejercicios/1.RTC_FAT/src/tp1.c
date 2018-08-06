
#include "rtc.h"   // <= own header (optional)
#include <string.h>

#include "sd_spi.h"   // <= own header (optional)
#include "sapi.h"     // <= sAPI header

#include "ff.h"       // <= Biblioteca FAT FS

#define FILE "Muestras.txt"

#define BUFFER_SIZE 200
static char uartBuff[10];
static FATFS fs;           // <-- FatFs work area needed for each volume
static FIL fp;             // <-- File object needed for each open file


// FUNCION que se ejecuta cada vezque ocurre un Tick
void diskTickHook( void *ptr ){
   disk_timerproc();   // Disk timer process
}

char* itoa(int value, char* result, int base) {
   // check that the base if valid
   if (base < 2 || base > 36) { *result = '\0'; return result; }

   char* ptr = result, *ptr1 = result, tmp_char;
   int tmp_value;

   do {
      tmp_value = value;
      value /= base;
      *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
   } while ( value );

   // Apply negative sign
   if (tmp_value < 0) *ptr++ = '-';
   *ptr-- = '\0';
   while(ptr1 < ptr) {
      tmp_char = *ptr;
      *ptr--= *ptr1;
      *ptr1++ = tmp_char;
   }
   return result;
}


/* Enviar fecha y hora en formato "DD/MM/YYYY, HH:MM:SS" */
void showDateAndTime( rtc_t * rtc ){
   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->mday), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio el dia */
   if( (rtc->mday)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, '/' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->month), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio el mes */
   if( (rtc->month)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, '/' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->year), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio el a������o */
   if( (rtc->year)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );


   uartWriteString( UART_USB, ", ");


   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->hour), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio la hora */
   if( (rtc->hour)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, ':' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->min), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio los minutos */
  // uartBuff[2] = 0;    /* NULL */
   if( (rtc->min)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, ':' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->sec), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio los segundos */
   if( (rtc->sec)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );


   /* Envio un 'enter' */
   uartWriteString( UART_USB, "\r\n");
}


/* FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE RESET. */
int main(void){

   /* ------------- INICIALIZACIONES ------------- */

   /* Inicializar la placa */
   boardConfig();

   /* Inicializar UART_USB a 115200 baudios */
   uartConfig( UART_USB, 115200 );

   adcConfig( ADC_ENABLE ); /* ADC */

   spiConfig( SPI0 );

   uint16_t muestra1 = 0;
   uint16_t muestra2 = 0;
   uint16_t muestra3 = 0;

     // Inicializar el conteo de Ticks con resolucion de 10ms,
     // con tickHook diskTickHook
     tickConfig( 10 );
     tickCallbackSet( diskTickHook, NULL );

     // ------ PROGRAMA QUE ESCRIBE EN LA SD -------

     UINT nbytes;

     // Give a work area to the default drive
     if( f_mount( &fs, "", 0 ) != FR_OK ){
        // If this fails, it means that the function could
        // not register a file system object.
        // Check whether the SD card is correctly connected
     }

     // Create/open a file, then write a string and close it

     uint8_t i=0;

     rtc_t rtc;

        rtc.year = 2016;
        rtc.month = 7;
        rtc.mday = 3;
        rtc.wday = 1;
        rtc.hour = 13;
        rtc.min = 17;
        rtc.sec= 0;

        bool_t val = 0;

        /* Inicializar RTC */
        val = rtcConfig( &rtc );

        delay_t delay1s;
        delayConfig( &delay1s, 1000 );

        delay(2000); // El RTC tarda en setear la hora, por eso el delay

        for( i=0; i<10; i++ ){
           val = rtcRead( &rtc );
           showDateAndTime( &rtc );
           delay(1000);
        }

        rtc.year = 2018;
        rtc.month = 7;
        rtc.mday = 3;
        rtc.wday = 1;
        rtc.hour = 14;
        rtc.min = 30;
        rtc.sec= 0;

        /* Establecer fecha y hora */
        val = rtcWrite( &rtc );

   /* ------------- REPETIR POR SIEMPRE ------------- */

   char buffer[BUFFER_SIZE]={0};
   while(1) {

      if( delayRead( &delay1s ) ){
          muestra1 = adcRead( CH1 );
          muestra2 = adcRead( CH2 );
          muestra3 = adcRead( CH3 );

         val = rtcRead( &rtc );
         snprintf(buffer,BUFFER_SIZE,"%d;%d;%d;%d/%d/%d_%d:%d:%d\r\n",muestra1,muestra2,muestra3,rtc.year,rtc.month,rtc.mday,rtc.hour,rtc.min,rtc.sec);
         //snprintf(buffer,BUFFER_SIZE,"%d/%d/%d/%d_%d:%d:%d\r\n",rtc.year,rtc.month,rtc.mday,rtc.hour,rtc.min,rtc.sec);

         if( f_open( &fp, FILE, FA_WRITE | FA_OPEN_APPEND ) == FR_OK ){
                   //f_write( &fp, "Hola mundo\r\n", 12, &nbytes );
        	 	 	 f_write( &fp, buffer, strlen(buffer), &nbytes );
        	 	    uartWriteString( UART_USB, buffer );


                   f_close(&fp);

                   if( nbytes == strlen(buffer) ){
                      // Turn ON LEDG if the write operation was successful
                      gpioToggle( LEDG );
                   }
                } else{
                   // Turn ON LEDR if the write operation was fail
                   gpioWrite( LEDR, ON );
                }
        // showDateAndTime( &rtc );
      }
      buffer[0]=0;

   }

   /* NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa no es llamado
      por ningun S.O. */
   return 0 ;
}

/*==================[end of file]============================================*/
