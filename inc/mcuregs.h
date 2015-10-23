/*---------------------------------------------------------------------------
  Tutorial application for STM8 devices

 *---------------------------------------------------------------------------
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *---------------------------------------------------------------------------

  Copyright (c) 2009 STMicroelectronics

	It's recommended to include mcuregs.h in all source files, instead
	of the device-specific header file directly, so	that changing the
	microcontroller	will be limited to a single change here
 ----------------------------------------------------------------------------*/


 /* Mutual-exclusive defines for STM8 family selection: choose most appropriate.
   Differences between families impacting this application:
    - vector interrupt mapping
		- number of channels of timer2
		- peripheral clock activation
*/

//#define STM8S_FAMILY /* For STM8S or STM8A families */
#define STM8L_FAMILY

#ifdef STM8S_FAMILY
#include "stm8s208mb.h"
#endif


#ifdef STM8L_FAMILY
#include "stm8l101g3u.h"

/* Macro for enabling Timer2 clock: select the register name according to the
   selected micro */
#define STM8L_TIMER2_CLK_ENR CLK_PCKENR    /* STM8L101x */
//#define STM8L_TIMER2_CLK_ENR CLK_PCKENR1 /* STM8L15xx */

#endif