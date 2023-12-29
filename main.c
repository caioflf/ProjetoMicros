#include <avr/io.h>
#include <avr/interrupt.h>

#define SENHA1 "1234"
#define SENHA2 "4321"
//display PORTC
//teclado PORTD e B
#define RS 4
#define EN 5
#define linha_max 7
#define botao1 2
#define botao2 1
#define botao3 0

unsigned char teclado[4][3]={'1','2','3',
	'4','5','6',
	'7','8','9',
'*','0','#'};
unsigned int pos_carro_x;
unsigned int pos_carro_y;
unsigned char serial_global[12]={'\0'};
unsigned char contador_global=0;

struct posCarro{
	int x;
	int y;
}pos_carro;

struct cliente{
	char cod;
	int pos_saida_x;
	int pos_saida_y;
	int pos_destino_x;
	int pos_destino_y;
}cliente1;


void atraso_1ms(){
	TCCR1B = 10;				// CTC prescaler de 1
	OCR1A = 2000;				// 2000 contagens (1ms)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag de estouro
}
void atraso_1ms64(){
	TCCR1B = 10;				// CTC prescaler de 1
	OCR1A = 3280;				// 3280 contagens (1.64ms)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag de estouro
}
void atraso_15ms(){
	TCCR1B = 10;				// CTC prescaler de 1
	OCR1A = 30000;				// 30000 contagens (15ms)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag de estouro
}
void atraso_40us(){
	TCCR1B = 9;					// CTC prescaler de 8
	OCR1A = 640;				// 640 contagens (40us)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag
}
void comando_lcd (unsigned char comando){ // comando em 4bits
	PORTC &= ~(1<<RS);			// RS = 0
	PORTC |= (1<<EN);			// EN = 1
	PORTC &= 0x30;				// Limpa porta -> 0011 0000
	PORTC |= ((comando)/16);	//Parte alta do comando
	PORTC &= ~(1<<EN);			// EN = 0
	PORTC |= (1<<EN);
	PORTC &= 0x30;
	PORTC |= (comando&0x0F);	// Parte baixa do comando
	PORTC &= ~(1<<EN);
	atraso_40us();
}

void letra_lcd (unsigned char comando){ // letra em 4bits
	PORTC |= (1<<RS);					// RS = 1
	PORTC |= (1<<EN);					// EN = 1
	PORTC &= 0x30;						// Limpa porta -> 0011 0000
	PORTC |= ((comando)/16);			//Parte alta do comando
	PORTC &= ~(1<<EN);					// EN = 0
	PORTC |= (1<<EN);
	PORTC &= 0X30;
	PORTC |= (comando&0x0F);			// Parte baixa do comando
	PORTC &= ~(1<<EN);
	atraso_40us();
}
void escreve_lcd (char  *msg){ // escreve um string no lcd
	unsigned char i=0;
	while (msg[i] != 0){
		letra_lcd(msg[i]);
		i++;
	}
}
void inicia_lcd_4bits(){ // inicializa em 4bits o lcd
	atraso_15ms();
	comando_lcd (0x28);
	comando_lcd (0x0C);
	comando_lcd (0x06);
	comando_lcd (0x01);
	atraso_1ms64();
}

unsigned char debounce(unsigned char num_bit){
	unsigned char cont=0;
	unsigned char ultimo=0;

	while(cont<7){
		atraso_1ms();
		if(ultimo==(PINB&(1<<num_bit))){
			cont++;
		}
		else{
			ultimo=(PINB&(1<<num_bit));
			cont=0;
		}
		
	}
	return ultimo;
}

unsigned char scan(unsigned char linha){//linhas 0, 1, 2 e 3
	PORTD&=~(1<<(linha_max-linha));
	if(!debounce(botao1)){
		while(!(PINB&(1<<botao1)));
		PORTD|=(1<<(linha_max-linha));
		return teclado[linha-1][0];
	}
	if(!debounce(botao2)){
		while(!(PINB&(1<<botao2)));
		PORTD|=(1<<(linha_max-linha));
		return teclado[linha-1][1];
	}
	if(!debounce(botao3)){
		while(!(PINB&(1<<botao3)));
		PORTD|=(1<<(linha_max-linha));
		return teclado[linha-1][2];
	}

	PORTD|=(1<<(linha_max-linha));
	return '\0';
}
void limpa_lcd(){
	comando_lcd(0x01);
	atraso_1ms64();
}
void config_serial(){
	UBRR0H = 0;
	UBRR0L = 51;						// 19200bps
	UCSR0B = (1<<4);					// liga serial
	UCSR0C = 0x06;						// 8bits +1 stopbit -> sem paridade
}
unsigned char le_serial(){				// funcao de leitura serial -> nao usada
	while ((UCSR0A & (1<<RXC0)) == 0);	// enquanto nao recebe leitura, aguarda
	UCSR0A |= (1<<RXC0);				// limpa flag
	return UDR0;						// retorna leitura
}
void inicia(){
	DDRD = 0x78;			//0111 1000 -> inicia porta D
	DDRB = 0x00;
	PIND=255;				// pullup porta D
	PINC=255;				// pullup porta C
	PINB=255;				// pullup porta B
	DDRC = 0x3F;			// porta C como saida -> 00111111
	inicia_lcd_4bits();
	TCCR1A = 0;					// modo padrão do timer
	config_serial();
	sei();						// habilita interrupções
	UCSR0B |= (1 << RXCIE0);	// Habilita a interrupção de recepção
	limpa_lcd();
}
char compara_string(char* a, char* b) // 0 se igual, 1 se diferente
{
	char flag = 0;
	while (*a != '\0' || *b != '\0') {
		if (*a == *b) {
			a++;
			b++;
		}
		else if ((*a == '\0' && *b != '\0') || (*a != '\0' && *b == '\0') || *a != *b){// se forem diferentes
			flag = 1;
			return 1;
		}
	}
	if (flag == 0) { // se forem iguais
		return 0;
	}
}
char ler_senha(){
	char perfil = 0; //perfil 0 (senha invalida), perfil 1 (ADM), perfil 2 (OUTRO)
	char senha[10]={'\0'}, i=0, j=0, letra = '\0';
	while(letra!='#'){
		for(i=1;i<=4;i++){
			letra=scan(i);
			if('\0'!=letra && '*' != letra && '#' != letra){
				letra_lcd('*');
				senha[j] = letra;
				j++;
			}
			else if (letra == '*'){
				j = 0;
				limpa_lcd();
				escreve_lcd("Insira a senha:");
				comando_lcd(0xC0);
				while (senha[j+1]!='\0'){
					senha[j] = '\0';
					j++;
				}
				j=0;
			}
		}
	}
	if (!compara_string(senha, SENHA1)){
		perfil = 1;
		} else if (!compara_string(senha, SENHA2)){
		perfil = 2;
	}
	return perfil;
}
char login (){
	char perfil = 0;
	limpa_lcd();
	escreve_lcd("Insira a senha:");
	comando_lcd(0xC0); // nova linha
	while(!perfil){
		perfil = ler_senha();
		if (perfil == 1){
			limpa_lcd();
			escreve_lcd("Perfil 1 Logado!");
			} else if (perfil == 2){
			limpa_lcd();
			escreve_lcd("Perfil 2 Logado!");
			} else {
			limpa_lcd();
			escreve_lcd("Invalida.");
			comando_lcd(0xC0);
		}
	}
	
}
void escreve_serial(char dado) {
	while (!(UCSR0A & (1 << UDRE0)));  // Aguarda o buffer de transmissão ficar vazio
	UDR0 = dado;                        // Coloca o byte no buffer de transmissão
}
void string_serial (char  *msg){ 	// escreve um string no serial
	unsigned char i=0;
	while (msg[i] != 0){
		escreve_serial(msg[i]);
		i++;
	}
}

void limpa_serial_global(){
	char i=0;
	while(serial_global[i]!='\0'){
		serial_global[i]='\0';
		i++;
	}
	contador_global=0;
}

void interpreta_serial(){
	if (serial_global[0] == 'S' && serial_global[1] == 'P' && serial_global[5] !='\0'){
		pos_carro.x=serial_global[2]*16+serial_global[3];
		pos_carro.y=serial_global[4]*16+serial_global[5];
		string_serial("UP");
		limpa_serial_global();
		}else if(serial_global[0] == 'S' && serial_global[1] == 'C' && serial_global[10] !='\0'){
		cliente1.cod=serial_global[2];
		cliente1.pos_saida_x=serial_global[3]*16+serial_global[4];
		cliente1.pos_saida_y=serial_global[5]*16+serial_global[6];
		cliente1.pos_destino_x=serial_global[7]*16+serial_global[8];
		cliente1.pos_destino_y=serial_global[9]*16+serial_global[10];
		string_serial("UC");
		limpa_serial_global();
		}else if(serial_global[0] == 'S' && serial_global[1] == 'M'){
		limpa_serial_global();
		}else if(serial_global[0] == 'S' && serial_global[1] == 'E'){
		limpa_serial_global();
		}else if(serial_global[0] == 'S' && serial_global[1] == 'A' && serial_global[2] =='C'){
		limpa_serial_global();
		}else if(serial_global[0] == 'S' && serial_global[1] == 'A' && serial_global[2] =='X'){
		limpa_serial_global();
		}else if(serial_global[0] == 'S' && serial_global[1] == 'I'){
		limpa_serial_global();
		}else if(serial_global[0] == 'S' && serial_global[1] == 'E'){
		limpa_serial_global();
	}
}




int main(void){
	char i, letra='\0', perfil;
	inicia();
	perfil = login();
	while (1) {

	}
}

ISR(USART_RX_vect){ // interrupção de recebimento serial
	serial_global[contador_global] =  UDR0;
	UCSR0A |= (1<<RXC0);				// limpa flag
	if(contador_global>=1){
		interpreta_serial();
	}
	contador_global++;
}
