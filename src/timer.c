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
 ----------------------------------------------------------------------------*/

#include <stdlib.h>
#ifdef __RAISONANCE__
	#include <intrins.h>  /* For inline assembly instructions declaration */
#endif
#include "mcuregs.h"  /* Device-dependent register map */

/*---------------------------------------------------------------
  Constants
 --------------------------------------------------------------*/
   
/* Compiler-specific definitions */
#ifdef __CSMC__
	#define asm_trap() _asm ("trap\n")
	#define asm_wfi()  _asm ("wfi\n")
	#define enable_interrupts() _asm ("rim\n")
#endif

#ifdef __RAISONANCE__
	#define asm_trap() _trap_()
	#define asm_wfi()  _wfi_()
	#define enable_interrupts()  _rim_()
#endif

	
/* Constants */                        
#define MAX_OCCURENCE_BEFORE_SWITCH_OFF 0xFFFF 

#define STM8_TIMx_CR1_OPM   0x08
#define STM8_TIMx_CR1_UDIS  0x02
#define STM8_TIMx_CR1_CEN   0x01
#define STM8_TIMx_SR1_CC1IF 0x02
#define STM8_TIMx_CC1IE     0x02
#define STM8_TIMx_EGR_UG    0x01
#define STM8L_PCKEN_TIM2    0x01

/*---------------------------------------------------------------
  Types definition
 --------------------------------------------------------------*/
 typedef enum
{
  GREEN,
  ORANGE,
  RED,
  OFF
} color;

typedef struct
{
  /* Time to spend in the state (in timer ticks) */
  const unsigned short timeoutValue;
  /* Number of times we reach the state */
  unsigned short stateOccurence;
} t_state;

/*---------------------------------------------------------------
  Global variables
 --------------------------------------------------------------*/
/* Current state */
static volatile color currentState=GREEN;

/* Number of transitions */
static unsigned long nbOfTransitions=0;

/* Definition of the state structure */
static t_state state[4]={ {0x4CCC, 1}, /* green  */
                          {0x1000, 0}, /* orange */
                          {0x4CCC, 0}, /* red    */
                          {0xFFFF, 0}  /* off    */ };
	

/* Set this value to 1 with the debugger in order to simulate a bug */
static unsigned char enableBug=0; 
static unsigned char bugCountDown;


/*---------------------------------------------------------------
   Trap Interrupt Service Routine: manage state transition
 --------------------------------------------------------------*/
#ifdef __CSMC__
@interrupt void trapISR (void )
#endif
#ifdef __RAISONANCE__
void trapISR (void ) trap
#endif
{
  color nextState;  
  
  /* Further interrupts are masked by hardware at this stage */

  /* Determine next state */
  switch(currentState)
  {
    case RED:
      nextState=GREEN;
      break;
    case OFF:
      nextState=OFF;
      break;
    default:
      nextState=currentState+1;
      break;
  }

  /* Go to next state and update traced variables */
  if( state[nextState].stateOccurence < MAX_OCCURENCE_BEFORE_SWITCH_OFF) 
  {
    currentState=nextState;
    state[currentState].stateOccurence++;
  }
  else
  {
    /* Stop transitions before overflow */
    currentState=OFF;
  }                              
  
  /* Update transition counter */
  nbOfTransitions++;

  return;
}

/*---------------------------------------------------------------
   Timer2 Interrupt Service Routine: used to wake up out of WFI
 --------------------------------------------------------------*/
#ifdef __CSMC__
@interrupt void timer2compareISR (void)
#endif
#ifdef __RAISONANCE__
	#ifdef STM8S_FAMILY
		void timer2compareISR (void ) interrupt 14
	#endif
	#ifdef STM8L_FAMILY
		void timer2compareISR (void ) interrupt 20
	#endif
#endif
{
	/* Acknowledge the interrupt */
	if( TIM2_SR1 & STM8_TIMx_SR1_CC1IF )
	{
		TIM2_SR1 &= ~STM8_TIMx_SR1_CC1IF;
	}
	return;
}

/*---------------------------------------------------------------
   Startup initialization of timer2
 --------------------------------------------------------------*/
void initTimer(void)
{
	/* Use Timer2 channel1 as output compare; generate an IT that will wake up
	  the micro out of WFI */
#ifdef STM8L_FAMILY
	/* Timer2 clock is not enabled by default */
	STM8L_TIMER2_CLK_ENR |= STM8L_PCKEN_TIM2;
#endif
	/* Enable channel 1 compare interrupt */
	TIM2_IER = STM8_TIMx_CC1IE;
	/* Avoid common application values in unused channels (not mandatory step) */
	TIM2_CCR2H = 0xFF;
	TIM2_CCR2L = 0xFF;
#ifdef STM8S_FAMILY
	TIM2_CCR3H = 0xFF;
	TIM2_CCR3L = 0xFF;
#endif
}

/*---------------------------------------------------------------
  Temporisation loop, using WFI and Timer2 interrupt
 --------------------------------------------------------------*/
void waiting_loop(const unsigned short count)
{
	/* Set number of cycles to wait before interrupt */
	TIM2_CCR1H = (unsigned char)((count & 0xFF00)>>8);
	TIM2_CCR1L = (unsigned char)((count & 0x00FF));
	/* Reinit the counter before enable */
	TIM2_EGR = STM8_TIMx_EGR_UG;
	/* Enable counter in one pulse mode without update event */
	TIM2_CR1 = STM8_TIMx_CR1_OPM | STM8_TIMx_CR1_UDIS | STM8_TIMx_CR1_CEN;
	/* Set the microcontroller in active low power mode until interrupt */
	asm_wfi();
}

/*---------------------------------------------------------------
  Pseudo-random variable initialization
 --------------------------------------------------------------*/
void reset_bug_count_down(void)
{
  /* Initialize to a value between 5 and 10 */
  bugCountDown = 5 + rand()%5;
}

/*---------------------------------------------------------------
  Main loop generating trap interrupts after a delay
 --------------------------------------------------------------*/
void main_loop(void)
{
  unsigned char *pData = (unsigned char *)&nbOfTransitions;
  
  while(1)
  {                     
    /* Wait for a moment in the current state */
    waiting_loop(state[currentState].timeoutValue);
    
    /* Generate a bug */
    if (enableBug)
    {
      if (bugCountDown == 0)
      {
        pData[1] = 0xaa; /* This will overwrite the variable <nbOfTransitions> */
        /* Reset the count down */
        reset_bug_count_down();
      }
      bugCountDown--;
    }
    
    /* generate an interrupt */
    asm_trap();
  }
}

/*--------------------------------------------------------------- 
  User application entry point
 --------------------------------------------------------------*/
void main( void )
{
  reset_bug_count_down();
  initTimer();
	enable_interrupts();
	main_loop();
  return;
}
