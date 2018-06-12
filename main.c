#include <msp430x14x.h>
#include <stdbool.h>
#include <in430.h>
#include <intrinsics.h>
#include <msp430f1611.h>
#include <stdint.h>

int iterator = 0;
int stopping = 0;
int state = 0;
char* START = "\n\rSTART\n";
uint16_t value_DMASZ;


int main( void )
{
 	WDTCTL = WDTPW + WDTHOLD;

//UART---------------------------------------------------------------------------

	DCOCTL = DCO0;  //ustawienie zegara SMCLK
	BCSCTL1 |= RSEL2 + RSEL0;  //SMCLK

	U0CTL |= SWRST; // Wlaczenie software reset

	U0CTL |= CHAR; // 8-bit character
	U0TCTL |= SSEL1; // Source clock SMCLK

	ME1 |= UTXE0 | URXE0; // Enabled USART0 TXD RXD

	// Ustawienia dla 115200
	U0BR0 = 0x09;
	U0BR1 = 0x00;
	U0MCTL = 0x08;

	U0CTL &= ~SWRST; // Wylaczenie software reset

	IE1 |= UTXIE0;
	// DMA0 transmiting
	DMACTL0 |= DMA0TSEL_4; // transmit trigger transmit
	DMA0CTL |= DMASRCINCR_3 + DMASRCBYTE + DMADSTBYTE + DMALEVEL + DMAIE; // increment source, byte in, byte out, level triggered

	// DMA1 recv
	DMACTL0 |= DMA1TSEL_3; // recv trigger recv
	DMA1CTL |= DMADSTINCR_3 + DMASRCBYTE + DMADSTBYTE + DMALEVEL + DMAIE; // increment source, byte in, byte out, level triggered


//UART koniec---------------------------------------------------------------------

	P3SEL = BIT4 + BIT5;
	P3DIR = BIT4;
	_EINT();
	DMA1SA = (unsigned int)&U0RXBUF; // Source block address
	DMA1DA = 0x1100; // Dest single address
	DMA1SZ = 1; // Block size
	DMA1CTL |= DMAEN;
	drukuj (START);
	while(true)
	{
		if(state == 0){
			getready4size();
		}
		if(state == 1){
			getready4file();
		}
		if (state == 2){
			runprogram();
		}
		//__bis_SR_register(LPM3_bits | GIE);
	}
	return 0;
}

inline void runReceivedProgram() {
	typedef int func(void);
	func* main = (func*)0x1200;
	main();
	(*(void(**)(void))(0xfffe))();
}

inline void runprogram(){

}

inline void getready4size(){
	DMA1SZ = 2;
	DMA1CTL |= DMAEN;
	DMA1DA = &value_DMASZ;
	__bis_SR_register(LPM3_bits | GIE);
}

inline void getready4file(){
	DMA1SZ = value_DMASZ;
	DMA1CTL |= DMAEN;
	DMA1DA = 0x1000;
	__bis_SR_register(LPM3_bits | GIE);
}


inline void drukuj (char * napis){
	iterator = 0;
	while (1){
		if (napis[iterator]=='\0'){
			iterator = 0;
			break;
		}
		else{
			stopping = 1;
			U0TXBUF = napis[iterator];
			while (stopping){}

		}
	}
}


#pragma vector = USART0RX_VECTOR // RECEIVER
__interrupt void usart0_rx (void)
{
	int i;
   LPM0_EXIT;
   U0TXBUF = 'L';
}

#pragma vector = USART0TX_VECTOR // TRANSMITER
__interrupt void usart0_tx (void)
{
	iterator++;
	stopping = 0;
}
#pragma vector=DACDMA_VECTOR
__interrupt void dmaisr(void)
{
	if(state == 0) state = 1;
	else if(state == 1) state = 2;
	_BIC_SR_IRQ(LPM3_bits);
}
