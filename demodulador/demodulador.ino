/***********************************************************************************/
/*                 AUTOBOTZ - ACADEMICS TEAM OF AUTONOMOS ROBOTICS                 */
/*                   UNIVERSIDADE FEDERAL DE MINAS GERAIS - UFMG                   */
/*                                                                                 */
/*                       Autor Principal: Rafael Murakami                          */
/*                                                                                 */
/***********************************************************************************/

/*Firmware desenvolvido com base no application note AVR135 e adaptado do codigo for-
necido pela ATMEL de autoria de Bruce McKenny. Este firmware faz uso do Input Capture
pin (ICP) do timer1 do atmega328P, que no arduino uno esta disponivel no pino 8.*/

#include <avr/io.h>
#include <avr/interrupt.h>

#define	ICP_SCALE    65536 //for 2^16 bits
#define TAM   10

volatile unsigned int icp_start_time = 0, icp_stop_time = 0;
volatile byte index = 0, indexF = 0;
volatile unsigned int vector[TAM];
volatile unsigned int vector_falling[TAM];
volatile boolean finish = false, finishF = false;
volatile byte nop;

void setup()
{
  pinMode(8, INPUT);
  setup_timer1();
  Serial.begin(115200);
}

void loop()
{  
    
  if(finish)
  {
    TIMSK1 = 0; //Cancela as interrupções geradas pelo ICP após guardar o número limite de dados
    int i;
    for(i = 0; i < TAM; i++)
    {
      Serial.print("INDICE ** ");
      Serial.println(i);
      Serial.println(vector[i]);
      
    }
    Serial.println("");
        for(i = 0; i < TAM; i++)
    {
      Serial.print("FALL ** ");
      Serial.println(i);
      Serial.println(vector_falling[i]);
    }
      
    Serial.print("******************* FIM ******************");
    Serial.println("");
    while(1);
  }
      
}

void setup_timer1(void)
{
  //Inicialização dos registradores do timer1
  TCCR1A = 0; //registrador de configuração A
  TCCR1B = 0; //registrador de configuração B
  OCR1A = 0; //registrador A de comparação de saída
  TIMSK1 = 0; //registrador de interrupção
  
  /*Configuração do registrador B para ativar evento de captura (ICP) - Iput Capture pin setando ICES1
  * Input Capture Edge Select (1 = rising edge, 0 = falling edge)
  * e para configurar prescaler
  * c - timer1 stopped
  * ((0 << CS12) | (0 << CS11) | (1 << CS10)) - clk/1
  * ((0 << CS12) | (1 << CS11) | (0 << CS10)) - clk/8
  * ((0 << CS12) | (1 << CS11) | (1 << CS10)) - clk/64
  * ((1 << CS12) | (0 << CS11) | (0 << CS10)) - clk/256
  * ((1 << CS12) | (0 << CS11) | (1 << CS10)) - clk/1024
  */
  TCCR1B |= ((0 << CS12) | (1 << CS11) | (1 << CS10)) | (1 << ICES1);
  
  /*Configuração de interrupção pelo ICP setando ICIE1 (Input Capture Interrupt Enable) e OCR1A
  * A interrupção por OCR1A é um timeout para detectar 0% ou 100% fora do período esperado
  */
  TIMSK1 |= (1 << ICIE1) | ( 1 << OCIE1A);
}

ISR(TIMER1_CAPT_vect)
{
  unsigned int timestamp, duty_cycle;
  unsigned char aux_regB; //armazena valor do registrador de configuração B antes de inveter o bit de detecção de borda
  
  //Salva o timestamp do pulso detectado
  timestamp = ICR1;
  
  //Inverte o tipo de detecção de borda para poder computar o duty cycle do pulso
  aux_regB = TCCR1B;
  TCCR1B = aux_regB ^ (1 << ICES1); //operação de OU exclusivo (XOR)
    
  /*Testa se foi detectado borda de subida
  * Faz uma operação AND com o estado do registrador B antes da inversão com o bit de ICES setado em 1,
  * que é equivalente a detecção de borda de subida
  */
  if((aux_regB & (1 << ICES1)) == (1 << ICES1))
  {
    icp_start_time = timestamp;		    /* Start of new pulse/period */
    //duty_cycle = icp_stop_time - icp_start_time;  /* Length of previous pulse */
    duty_cycle = icp_start_time;
     
    if (index >= TAM)
      nop = 0;
    else
    {
      vector[index] = duty_cycle;
      index++;
    }       
  }
  else
  {
    //Falling edge detected
    icp_stop_time = timestamp;
    
    if (indexF >= TAM)
      finish = true;
    else
    {
      vector_falling[indexF] = icp_stop_time;
      indexF++;
    }            
  }
    
}


