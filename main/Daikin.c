/* Daikin app */
/* Copyright �2022 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details .GPL 3.0 */

static __attribute__((unused))
const char TAG[] = "Daikin";

#include "revk.h"
#include "esp_sleep.h"
#include "esp_task_wdt.h"
#include <driver/gpio.h>
#include <driver/uart.h>

#define PORT_INV 0x40
#define port_mask(p) ((p)&63)

// Settings
#define	settings		\
	b(debug)	\
	u8(uart,1)	\
	io(tx,)	\
	io(rx,)	\

#define u32(n,d)        uint32_t n;
#define s8(n,d) int8_t n;
#define u8(n,d) uint8_t n;
#define b(n) uint8_t n;
#define s(n) char * n;
#define io(n,d)           uint8_t n;
settings
#undef io
#undef u32
#undef s8
#undef u8
#undef b
#undef s
// Functions to actually talk to the Daikin
struct {                        // The current status based on messages received
   char model[20];              // Model number of attached unit
   char mode;                   // Current mode
   char fan;                    // Current fan speed
   uint8_t on:1;                // Currently on
} status;

struct {                        // The command status we wish to send
   float temp;                  // Target temperature
   char mode;                   // Mode to set ('A'=auto, 'H'=heat, 'C'=cool, 'D'=Dry, 'F'=Fan)
   char fan;                    // Fan speed '1' to '5'
   uint8_t on:1;                // Switched on
} command;



// --------------------------------------------------------------------------------
const char *app_callback(int client, const char *prefix, const char *target, const char *suffix, jo_t j)
{                               // MQTT app callback
   if (client || !prefix || target || strcmp(prefix, prefixcommand) || !suffix)
      return NULL;              //Not for us or not a command from main MQTT
   char value[1000];
   int len = 0;
   if (j)
   {
      len = jo_strncpy(j, value, sizeof(value));
      if (len < 0)
         return "Expecting JSON string";
      if (len > sizeof(value))
         return "Too long";
   }

   return NULL;
}

void app_main()
{
   revk_boot(&app_callback);
#define io(n,d)           revk_register(#n,0,sizeof(n),&n,"- "#d,SETTING_SET|SETTING_BITFIELD);
#define b(n) revk_register(#n,0,sizeof(n),&n,NULL,SETTING_BOOLEAN);
#define u32(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define s8(n,d) revk_register(#n,0,sizeof(n),&n,#d,SETTING_SIGNED);
#define u8(n,d) revk_register(#n,0,sizeof(n),&n,#d,0);
#define s(n) revk_register(#n,0,0,&n,NULL,0);
   settings
#undef io
#undef u32
#undef s8
#undef u8
#undef b
#undef s
       revk_start();
   {                            // Init uart
      esp_err_t err = 0;
      // Init UART for Mobile
      uart_config_t uart_config = {
         .baud_rate = 9600,
         .data_bits = UART_DATA_8_BITS,
         .parity = UART_PARITY_DISABLE,
         .stop_bits = UART_STOP_BITS_1,
         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      };
      if (!err)
         err = uart_param_config(uart, &uart_config);
      if (!err)
         err = uart_set_pin(uart, port_mask(tx), port_mask(rx), -1, -1);
      if (!err && ((tx & PORT_INV) || (rx & PORT_INV)))
         err = uart_set_line_inverse(uart, ((rx & PORT_INV) ? UART_SIGNAL_RXD_INV : 0) | ((tx & PORT_INV) ? UART_SIGNAL_TXD_INV : 0));
      if (!err)
         err = uart_driver_install(uart, 1024, 0, 0, NULL, 0);
      if (err)
      {
         jo_t j = jo_object_alloc();
         jo_string(j, "error", "Failed to uart");
         jo_int(j, "uart", uart);
         jo_int(j, "gpio", port_mask(rx));
         jo_string(j, "description", esp_err_to_name(err));
         revk_error("uart", &j);
         return;
      }
   }

   while (1)
   {
      // TODO
      sleep(1);
   }
}
