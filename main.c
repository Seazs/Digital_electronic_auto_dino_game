/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include "keypad.h"
#include "stdio.h"
#include "math.h"


long int score;
int is_score_augmenting = 0;
int automod = 0;
int lecture_photores;

int j = 0;
int k;
int vector[100];


CY_ISR (isr_son){
    //isr lié au timer qui permet de jouer les notes a la fréquence du timer
    k = vector[j];
    j++;
    k = k+1;
    k = k/2;
    k = 255*k;
    VDAC8_1_SetValue(k);
    if(j == 100){
        j = 0;
    }
    Timer_son_ReadStatusRegister();
}
void saut(){
    // fonction de saut
    PWM_saut_WriteCompare(4000); // Active le servomoteur
    
    Timer_son_WritePeriod(545); // Configure le timer pour avoir une fréquence correspondant à un la
    isr_son_Enable(); // active l'isr du son
    
    
    
    Pin_LED1_Write(1); // Allume les leds
    Pin_LED2_Write(1); // Allume les leds
    
    // Ecrit jump sur l'écran et sur les ports séries
    LCD_Char_Position(0,0);
    //LCD_Char_ClearDisplay();
    LCD_Char_PrintString("jump!");
    UART_PutString("jump\n");

    // Attend que le servo ait atteint sa position, éteint les leds et désactive l'isr du son
    CyDelay(200);
    PWM_saut_WriteCompare(4800);
    Pin_LED1_Write(0);
    Pin_LED2_Write(0);
    isr_son_Disable();
}
void bas(){
    // fonction de duck
    PWM_bas_WriteCompare(4000); // active le servo moteur
    
    Timer_son_WritePeriod(900); // Configure le timer pour avoir une autre fréquence qu'un la
    isr_son_Enable(); // active l'isr du son
    
    
    Pin_LED3_Write(1); // allume les leds
    Pin_LED4_Write(1); // allume les leds

    // Ecrit duck sur l'écran et sur les ports séries
    LCD_Char_Position(0,0);
    //LCD_Char_ClearDisplay();
    LCD_Char_PrintString("duck!");
    UART_PutString("duck\n");

    // Attend que le servo ait atteint se position, éteint les leds et désactive l'isr du son
    CyDelay(200);
    PWM_bas_WriteCompare(4800);
    Pin_LED3_Write(0);
    Pin_LED4_Write(0);
    isr_son_Disable();
}
CY_ISR (isr_saut){
    //interrupt lié au bouton1 qui saut et commence l'incrémenation du score s'il elle ne l'est pas encore
    if (is_score_augmenting == 0){
        Timer_score_Start();
        is_score_augmenting = 1;
    }
    saut();
}
CY_ISR (isr_bas){
    //interrupte lié au bouton 4 qui saut
    bas(); 
}
CY_ISR (isr_score){
    //isr lié au timer du score qu incrémente le score de 10 toutes les secondes
    if (is_score_augmenting == 1){
        score +=1;}
    //imprime le score sur l'écran
    LCD_Char_Position(1,0);
    LCD_Char_PrintDecUint16(score);
    Timer_score_ReadStatusRegister();
}
CY_ISR (isr_Reset_score){
    //interrupte lié au bouton 3 qui reset le score a 0
    Timer_score_Stop();
    is_score_augmenting = 0;
    score = 0;
    //imprime 0 sur l'écran
    LCD_Char_ClearDisplay();
    LCD_Char_Position(1,0);
    LCD_Char_PrintDecUint16(score);
}
CY_ISR (isr_automod){
    //interrupt lié au bouton 2 qui passe du mode automatique qui active les fonctionnalité des photosenseur
    // au mode manuel qui laisse seulement les commandes manuelles
    Pin_LED1_Write(1);
    if (automod == 0){
        automod = 1;
        //imprime A pour auto
        LCD_Char_Position(1,7);
        LCD_Char_PutChar('A');
    }
    else{
        automod = 0;
        //imprime M pour manuel
        LCD_Char_Position(1,7);
        LCD_Char_PutChar('M');
    }
}
CY_ISR (isr_serial) {
    //interrupt lié a la réception d'info via les ports séries
    uint8_t status = 0;
    
    do {
        // Checks if no UART Rx errors
        status = UART_ReadRxStatus () ;
        if (( status & UART_RX_STS_PAR_ERROR ) |
            ( status & UART_RX_STS_STOP_ERROR ) |
            ( status & UART_RX_STS_BREAK ) |
            ( status & UART_RX_STS_OVERRUN ) ) 
            {
             // Parity , framing , break or overrun error
             //LCD_Position (1 ,0) ;
             //LCD_PrintString ( " UART err " ) ;
        }
        // Check that rx buffer is not empty and get rx data
        while ( ( status & UART_RX_STS_FIFO_NOTEMPTY ) != 0) 
        {

            //LCD_Char_Position(0,1);
            //saut si on recoit z
            //duck si on recoit 
            const char8 rxData = UART_GetChar();
            
            //LCD_Char_PutChar(rxData);
            if(rxData == 'z'){
                saut();
            }else if(rxData == 's'){
                bas();
            }
        }
        
    } while (( status & UART_RX_STS_FIFO_NOTEMPTY ) != 0) ;
}
void creatsin(){
    // crée un vecteur de sinus
    for(int i=0; i<100; i++){
       vector[i] = sin(2*M_PI*i/100);
    }
}
void start_commande_bouton(){
    //start des pins des servo
    PWM_saut_Start();
    PWM_bas_Start();

    // start des isr
    isr_bas_StartEx(isr_bas);
    isr_saut_StartEx(isr_saut);
    isr_serial_StartEx(isr_serial);
    isr_score_StartEx(isr_score);
    isr_Reset_score_StartEx(isr_Reset_score);
    isr_automod_StartEx(isr_automod);
    isr_son_StartEx(isr_son);
    isr_son_Disable();

    // start du converter analog -> digital et du multiplexer
    ADC_DelSig_Start();
    AMux_Start(); 

    LCD_Char_Start();
    //Start de la partie audio
    VDAC8_1_Start();
    creatsin();

    UART_Start();
    Timer_score_Start();
    Timer_son_Start();
}
void commande_clavier(){
    // lis les pins des clavier et saut ou duck en focntion de la touche appuyé
    char c;
    c = keypadScan();
    if (c == '*'){
        saut();
    }
    if (c == '#'){
        bas();
    }
}
void commande_automatique(){
    //check si on est en mode auto
    if (automod == 1){  
        AMux_Select(0); // selection le photoresisteurs du bas
        CyDelay(10);
        ADC_DelSig_StartConvert(); //lis la valeur du photoresisteurs
        if(ADC_DelSig_IsEndConversion(ADC_DelSig_WAIT_FOR_RESULT)){
            lecture_photores = ADC_DelSig_GetResult32();
            lecture_photores = lecture_photores/655; // transforme pour avoir un nombre entre 0 et 100
            }
        if (lecture_photores < 26 && lecture_photores != 0){
            saut(); // saut si on a un arbre
        }
                
        AMux_Select(1); // selection le photoresisteurs du haut
        CyDelay(10);
        ADC_DelSig_StartConvert();
        if(ADC_DelSig_IsEndConversion(ADC_DelSig_WAIT_FOR_RESULT)){
            lecture_photores = ADC_DelSig_GetResult32();
            lecture_photores = lecture_photores/655;
            }
        if (lecture_photores < 26 && lecture_photores != 0){
            bas();
        }
    }
}


int main(void){
    CyGlobalIntEnable; /* Enable global interrupts. */
    
    start_commande_bouton(); // démarre tout les composants physique et initalise les variables nécéssaire   
    
    
    for(;;)
    {
        commande_clavier();
        commande_automatique();
    }
}

/* [] END OF FILE */
