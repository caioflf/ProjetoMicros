#include <avr/io.h>
#include <avr/interrupt.h>

#define SENHA1 "1234"
#define SENHA2 "000000"
//display PORTC
//teclado PORTD e B
#define RS 4
#define EN 5
#define linha_max 7
#define botao1 2
#define botao2 1
#define botao3 0
#define FB
unsigned char teclado[4][3]={'1','2','3',
	'4','5','6',
	'7','8','9',
'*','0','#'};

unsigned char serial_global[12]={'\0'};
unsigned char contador_global=0;

unsigned short ESQUINAS [12][2] = {
	420, 540,
	860, 540,
	1330, 540,
	1790, 540,
	420, 1060,
	860, 1060,
	1330, 1060,
	1790, 1060,
	420, 1580,
	860, 1580,
	1330, 1580,
1790, 1580 };

typedef struct rua{
	short dist;
	char indice;
}Rua;

Rua ruaGlobal;

typedef struct posCarro{
	short x;
	short y;
}posCarro;

posCarro posCarroGlobal;

typedef struct cliente{
	char cod;
	short pos_saida_x;
	short pos_saida_y;
	short pos_destino_x;
	short pos_destino_y;
}cliente;

cliente bufferCliente;

char flagClienteGlobal;

void atraso_1ms(){
	TCCR1B = 10;				// CTC prescaler de 8
	OCR1A = 2000;				// 2000 contagens (1ms)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag de estouro
}
void atraso_1ms64(){
	TCCR1B = 10;				// CTC prescaler de 8
	OCR1A = 3280;				// 3280 contagens (1.64ms)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag de estouro
}
void atraso_15ms(){
	TCCR1B = 10;				// CTC prescaler de 8
	OCR1A = 30000;				// 30000 contagens (15ms)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag de estouro
}
void atraso_40us(){
	TCCR1B = 9;					// CTC prescaler de 1
	OCR1A = 640;				// 640 contagens (40us)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag
}
void atraso_1s(){
	TCCR1B = 13;				// CTC prescaler de 1024
	OCR1A = 15625;				// 15625 contagens (1s)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag de estouro
}
void atraso_2s(){
	TCCR1B = 13;				// CTC prescaler de 1024
	OCR1A = 31250;				// 31250 contagens (1s)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag de estouro
}
void atraso_500ms(){
	TCCR1B = 13;				// CTC prescaler de 1024
	OCR1A = 7813;				// 7813 contagens (500ms)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag de estouro
}
void config_serial(){
	UCSR0B |= (1 << RXCIE0);			// habilita interrupção serial
	UBRR0H = 0;
	UBRR0L = 51;						// 19200bps
	UCSR0B = (1<<4);					// liga serial
	UCSR0C = 0x06;						// 8bits +1 stopbit -> sem paridade
	UCSR0B |= (1 << TXEN0);				// transmissao serial
	UCSR0B |= (1 << RXEN0);				// recepcao serial
}

void desliga_serial(){
	UCSR0B &= 0x0;
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

unsigned char le_serial(){				// funcao de leitura serial -> nao usada
	while ((UCSR0A & (1<<RXC0)) == 0);	// enquanto nao recebe leitura, aguarda
	UCSR0A |= (1<<RXC0);				// limpa flag
	return UDR0;						// retorna leitura
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
void limpa_lcd(){
	comando_lcd(0x01);
	atraso_1ms64();
}
void inicia_lcd_4bits(){ // inicializa em 4bits o lcd
	atraso_15ms();
	comando_lcd (0x28);
	comando_lcd (0x0C);
	comando_lcd (0x06);
	comando_lcd (0x01);
	atraso_1ms64();
}
void desliga_lcd_4bits() {
	atraso_15ms();
	comando_lcd (0x08);
	atraso_1ms64();
}
void desligaSistema (char *desligaSistema) {
	if (*desligaSistema == 1){
		desliga_lcd_4bits();
		desliga_serial();
		*desligaSistema = 0;
	}
}
void ligaSistema(char *flagSistema) {
	if (*flagSistema == 0){
		inicia_lcd_4bits();
		escreve_lcd("Insira a senha:");
		comando_lcd(0xC0); // nova linha
		config_serial();
		UCSR0B |= (1 << RXCIE0);
		*flagSistema = 1;
		atraso_1s();		// atraso de 1seg pra nao ser lido o que estiver sendo pressionado logo apos iniciar
	}
}

void calculo_distancia(){
	// codigo para o calculo da distância
	
}

int estimagemPreco (short dist) {
	int preco = 200;
	preco += (12*dist)/100;  // preço do km percorrido
	preco += (dist/139)*5;   // preço do tempo (138,88 m/10s)
	return preco;
}

char aceitaCorrida (short dist, int preco){
	char comando;
	
	return comando;
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

unsigned char verificacao_tecla1(char tempo){ //tempo multiplicado por 2 em seg por conta de usar atraso de 0,5seg
	unsigned char cont=0;					// verifica se é pressionado e segurado por um certo tempo a tecla '*'
	unsigned char ultimo=0;
	while(cont<tempo){
		if(ultimo==(1<<botao1)){
			atraso_1ms();
			cont++;
		}
		else{
			if(ultimo==(PINB&(1<<botao1))){
				atraso_500ms();
				cont++;
			}
			else{
				ultimo=(PINB&(1<<botao1));
				cont=0;
			}
		}
	}
	return ultimo;
}

unsigned char verificacao_tecla2(char tempo){ //tempo multiplicado por 2 em seg
	unsigned char cont=0;					// verifica se é pressionado e segurado por um certo tempo a tecla '#'
	unsigned char ultimo=0;
	while(cont<tempo){
		if(ultimo == (1<<botao3)){			
			atraso_1ms64();						
			cont++;
		}
		else{
			if(ultimo == (PINB&(1<<botao3))){
				atraso_500ms();
				cont++;
			}
			else{
				ultimo = (PINB&(1<<botao3));
				cont=0;
			}
		}
	}
	return ultimo;
}

unsigned char verifica_login(){			//scan apenas da linha 4, com um "debounce" maior para verificar se apertou a tecla tempo suficiente
	
	PORTD&=~(1<<(3));
	if(!verificacao_tecla1(6)){
		PORTD|=(1<<(3));
		return teclado[3][0];
	}
	if(!verificacao_tecla2(1)){ // lembrar de mudar :D
		PORTD|=(1<<(3));
		return teclado[3][2];
	}
	PORTD|=(1<<(3));
	return '\0';
}

unsigned char scan(unsigned char linha){//linhas 0, 1, 2 e 3
	PORTD&=~(1<<(linha_max-linha));
	if(!debounce(botao1)){
		if(linha == 4){
			if(!verificacao_tecla1(6)){		//verifica se vem comando de desligar o sistema
				return 'd';
			}
		}
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

void inicia(){
	DDRD = 0x79;			//0111 1000 -> inicia porta D
	DDRB = 0x00;
	PIND=255;				// pullup porta D
	PINC=255;				// pullup porta C
	PINB=255;				// pullup porta B
	DDRC = 0x3F;			// porta C como saida -> 00111111
	TCCR1A = 0;					// modo padrão do timer
	config_serial();
	sei();						// habilita interrupções
	UCSR0B |= (1 << RXCIE0);	// Habilita a interrupção de recepção serial
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
	while(letra!='#'&&(j<10)){
		for(i=1;i<=4;i++){
			letra=scan(i);
			if (letra == 'd')
			return letra;			//se scan retornar valor atribuido ao comando de desligamento, interrompe a funcao aqui
			
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
		return perfil;
	}
	if (!compara_string(senha, SENHA2)){
		perfil = 2;
		return perfil;
	}
	else {
		perfil = 0;
		return perfil;
	}
	return perfil;
}
void movimento (){
	char letra, i;
	for(i=1;i<=4;i++){
		letra=scan(i);
		if (letra == '5'){
			limpa_lcd();
			letra_lcd('5');
			string_serial("UM");
			escreve_serial(0);
		} else if (letra == '2'){
			limpa_lcd();
			letra_lcd('2');
			string_serial("UM");
			escreve_serial(1);
		} else if (letra == '6'){
			limpa_lcd();
			letra_lcd('6');
			string_serial("UM");
			escreve_serial(2);
		} else if (letra == '4'){
			limpa_lcd();
			letra_lcd('4');
			string_serial("UM");
			escreve_serial(3);
		}		
	}	
}

void armazenaCliente(cliente *clientesEspera, char opcaoB){
	if (opcaoB == 1){
		clientesEspera[1] = bufferCliente;
		flagClienteGlobal = 0;
	}
	
}

char ubergs(char *flagSistema, char *opcaoB, char *motoristaOcupado, char flagPerfil){
	unsigned char verificacao = 0;
	cliente clientesEspera[5], clienteAtual;
	flagClienteGlobal = 0;
	
	while (1){
		verificacao = verifica_login();
		if (verificacao == '*'){
			desligaSistema(flagSistema);
			return 'd';
		}
		if (verificacao == '#'){
			limpa_lcd();
			escreve_lcd("Logoff realizado");
			atraso_2s();
			return 1;
		}
		movimento();
		if (flagClienteGlobal = 1){
			
		}
		
	}
}

void lcdEscreverSenha(){
	limpa_lcd();
	escreve_lcd("Insira a senha:");
	comando_lcd(0xC0); // nova linha
}

char login (char *flagSistema, char *opcaoB, char *motoristaOcupado){
	char perfil = 0;
	lcdEscreverSenha();
	while(1){
		perfil = ler_senha();
		if (perfil == 'd'){				//comando de desligar o sistema
			desligaSistema(flagSistema);
			return perfil;
		}
		if (perfil == 1){
			limpa_lcd();
			escreve_lcd("Perfil 1 Logado!");
			perfil = ubergs(flagSistema, opcaoB, motoristaOcupado, 1);
			lcdEscreverSenha();
		}
		if (perfil == 2){
			limpa_lcd();
			escreve_lcd("Perfil 2 Logado!");
			perfil = ubergs(flagSistema, opcaoB, motoristaOcupado, 2);
			lcdEscreverSenha();
		}
		if (perfil == 'd'){
			desligaSistema(flagSistema);
			return perfil;
		}
		if (perfil == 0){
			limpa_lcd();
			escreve_lcd("Senha Invalida!");
			atraso_2s();				// atraso em que o usuario espera pra poder digitar novamente a senha
			limpa_lcd();
			escreve_lcd("Insira a senha:");
			comando_lcd(0xC0);
		}
	}
}



void interpreta_serial(){ //interpreta as mensagens enviadas pelo servidor externo
	if (serial_global[0] == 'S' && serial_global[1] == 'P' && serial_global[5] !='\0'){	 //Protocolo de posiçao do veículo
		posCarroGlobal.x = serial_global[2]*16 + serial_global[3];
		posCarroGlobal.y = serial_global[4]*16 + serial_global[5];
		string_serial("UP");
		limpa_serial_global();
	}
	
	else if(serial_global[0] == 'S' && serial_global[1] == 'C' && serial_global[10] !='\0'){ //protocolo de chamada de novo cliente
		bufferCliente.cod = serial_global[2];
		bufferCliente.pos_saida_x = serial_global[3]*16 + serial_global[4];
		bufferCliente.pos_saida_y = serial_global[5]*16 + serial_global[6];
		bufferCliente.pos_destino_x = serial_global[7]*16 + serial_global[8];
		bufferCliente.pos_destino_y = serial_global[9]*16 + serial_global[10];
		flagClienteGlobal = 1;
		string_serial("UC");
		limpa_serial_global();
	}
	
	else if(serial_global[0] == 'S' && serial_global[1] == 'M'){		// protocolo resposta do servidor a comando de movimentaçao
		limpa_serial_global();
	}
	
	else if(serial_global[0] == 'S' && serial_global[1] == 'E'){		// protocolo resposta do servidor a comando de estado do veiculo
		limpa_serial_global();
	}
	
	else if(serial_global[0] == 'S' && serial_global[1] == 'A' && serial_global[2] =='C'){ //protocolo de resposta do servidor a comando de aceite - confirmado
		limpa_serial_global();
	}
	
	else if(serial_global[0] == 'S' && serial_global[1] == 'A' && serial_global[2] =='X'){ //protocolo de resposta do servidor a comando de aceite - não disponivel
		limpa_serial_global();
	}
	
	else if(serial_global[0] == 'S' && serial_global[1] == 'I'){		// protocolo de resposta do servidor a comando de inicio de corrida
		limpa_serial_global();
	}
	
	else if(serial_global[0] == 'S' && serial_global[1] == 'E'){		// protocolo de resposta do servidor a comando de fim de corrida
		limpa_serial_global();
	}
}
unsigned int distancia(int x1, int y1, int x2, int y2) {	// calcula dist^2 entre dois pontos
    return (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1);
}
unsigned int modulo (int x){					// modulo de um numero
    if (x < 0) {
        return -x;
    } else {
        return x;
    }
}
char esquinas_adjacentes (unsigned short x, unsigned short y, unsigned char vet[]){ // descobre as esquinas adjacentes a um ponto
    unsigned char i, qtd = 0, flag = 0;
    unsigned short aux = x;
    
    while(x >= 1 && x <= 2500 && flag == 0){
        x++;
        for (i = 0; i < 12; i++){
            if (x == ESQUINAS[i][0] && y == ESQUINAS[i][1]){
                vet[qtd] = i;
                qtd++;
                flag = 1;
            }
        }
    }
    flag = 0;
    x = aux;
    while(x >= 1 && x <= 2500 && flag == 0){
        x--;
        for (i = 0; i < 12; i++){
            if (x == ESQUINAS[i][0] && y == ESQUINAS[i][1]){
                vet[qtd] = i;
                qtd++;
                flag = 1;
            }
        }
    }
    flag = 0;
    x = aux;
    aux = y;
    while(y >= 1 && y <= 2500 && flag == 0){
        y++;
        for (i = 0; i < 12; i++){
            if (x == ESQUINAS[i][0] && y == ESQUINAS[i][1]){
                vet[qtd] = i;
                qtd++;
                flag = 1;
            }
        }
    }
    flag = 0;
    y = aux;
    while(y >= 1 && y <= 2500 && flag == 0){
        y--;
        for (i = 0; i < 12; i++){
            if (x == ESQUINAS[i][0] && y == ESQUINAS[i][1]){
                vet[qtd] = i;
                qtd++;
                flag = 1;
            }
        }
    }
    return qtd;
}
unsigned char escolhe_esquina(unsigned short x, unsigned short y, unsigned short x_final, unsigned short y_final){ // escolhe a esquina adjacente mais proxima do destino
    unsigned char i, esquina, qtd =0;;
    int menor_dist = 2147483647/2;
    unsigned char vet[4];
    qtd = esquinas_adjacentes(x,y,vet);
    for(i = 0; i< qtd; i++){
        if (menor_dist > distancia(ESQUINAS[vet[i]][0], ESQUINAS[vet[i]][1], x_final, y_final)){
            menor_dist = distancia(ESQUINAS[vet[i]][0], ESQUINAS[vet[i]][1], x_final, y_final);
            esquina = vet[i];
        }
    }
    return esquina;
}
unsigned char calcula_caminho (unsigned short x, unsigned short y, unsigned short x_final, unsigned short y_final, unsigned char trajeto[]){ // calcula as esquinas de um trajeto
    unsigned char atual, destino, qtd = 0, flag =0, i = 0;
    destino = escolhe_esquina(x_final,y_final,x,y);

    while (!flag){
        trajeto[i] = escolhe_esquina(x,y,x_final,y_final);
        x = ESQUINAS[trajeto[i]][0];
        y = ESQUINAS[trajeto[i]][1];
        i ++;
        if (x == ESQUINAS[destino][0] && y == ESQUINAS[destino][1]){
            flag = 1;
        }
    }
    return i;
}
unsigned int calcula_distancia(unsigned short x, unsigned short y, unsigned short x_final, unsigned short y_final){	// calcula a distancia de um trajeto
    unsigned char qtd, i, trajeto[10];
    unsigned int distancia;
    qtd = calcula_caminho (x, y, x_final, y_final, trajeto);
    
    distancia = modulo(x - ESQUINAS[trajeto[0]][0] + y - ESQUINAS[trajeto[0]][1]);
    distancia = distancia + modulo (x_final - ESQUINAS[trajeto[qtd-1]][0] + y_final - ESQUINAS[trajeto[qtd-1]][1]);

    for (i = 0; i < qtd-1; i++){
        distancia = distancia + modulo(ESQUINAS[trajeto[i]][0] - ESQUINAS[trajeto[i+1]][0] + ESQUINAS[trajeto[i]][1] - ESQUINAS[trajeto[i+1]][1]);
    }
    return distancia;
}
void gps (unsigned short x, unsigned short y, unsigned short x_final, unsigned short y_final){  		// indica o sentido que o carro deve seguir
    unsigned char vet[4], proxima, sentido;
    unsigned short x_prox, y_prox;
    unsigned int dist_destino;
    proxima = escolhe_esquina(x, y, x_final, y_final);
    // para ponto atual
    x_prox = ESQUINAS[proxima][0];
    y_prox = ESQUINAS[proxima][1];
    if (x == x_final){
        if (y < y_final){
            sentido = 'S';
        } else if (y > y_final){
            sentido = 'N';
        } else {
            sentido = 'D';
        }
        
    } else if (y == y_final){
        if(x < x_final){
            sentido = 'L';
        } else if (x > x_final){
            sentido = 'O';
        } else {
            sentido = 'D';
        }
        
    } else if (x > x_prox){
        sentido = 'O';
    } else if (x < x_prox){
        sentido = 'L';
    } else if (y > y_prox){
        sentido = 'N';
    } else if (y < y_prox){
        sentido = 'S';
    }
   // printf("%c", sentido); // substituir por escreve_lcd
    dist_destino = calcula_distancia(x,y,x_final,y_final);
    
    // para prox ponto
    if (x != x_final && y != y_final){
        x = x_prox;
        y = y_prox;
        proxima = escolhe_esquina(x, y, x_final, y_final);
        x_prox = ESQUINAS[proxima][0];
        y_prox = ESQUINAS[proxima][1];
        if (x == x_final){
        if (y < y_final){
            sentido = 'S';
        } else if (y > y_final){
            sentido = 'N';
        } else {
            sentido = 'D';
        }
    } else if (y == y_final){
        if(x < x_final){
            sentido = 'L';
        } else if (x > x_final){
            sentido = 'O';
        } else {
            sentido = 'D';
        }
    } else if (x > x_prox){
        sentido = 'O';
    } else if (x < x_prox){
        sentido = 'L';
    } else if (y > y_prox){
        sentido = 'N';
    } else if (y < y_prox){
        sentido = 'S';
    }
   // printf("\nEm %im, siga a %c", modulo(y-y_prox + x-x_prox), sentido); // substituir por escreve_lcd
    //printf("\nDestino a %im", dist_destino); // substituir por escreve_lcd
    }
}
/* EXEMPLO DOS CODIGOS ACIMA SENDO USADOS
int main() { 
    unsigned char i, qtd, vet[4], esquina_carro, esquina_pass1, esquina_pass2, esquina_desti, trajeto[10];
    unsigned short  x_carro = 1450, y_carro = 1060,
                    x_passageiro = 420, y_passageiro = 510,
                    x_desti = 480, y_desti = 1580,
                    x = 1200, y = 1060;
    
    esquina_carro = escolhe_esquina(x_carro, y_carro, x_passageiro, y_passageiro);
    esquina_pass1 = escolhe_esquina(x_passageiro, y_passageiro, x_carro, y_carro);
    esquina_pass2 = escolhe_esquina(x_passageiro, y_passageiro, x_desti, y_desti);
    esquina_desti = escolhe_esquina(x_desti, y_desti, x_passageiro, y_passageiro);
    //printf("\n\nEsquina Carro = %i\nEsquina Pass1 = %i\nEsquina Pass2 = %i\nEsquina Desti = %i\n\n", esquina_carro, esquina_pass1, esquina_pass2, esquina_desti);
    
    qtd = calcula_caminho(x_carro, y_carro, x_passageiro, y_passageiro, trajeto);
    for (i = 0; i < qtd; i++){
        printf("%i ", trajeto[i]); // substituir por escreve_lcd
    }

    qtd = calcula_caminho(x_passageiro, y_passageiro, x_desti, y_desti, trajeto);
    for (i = 0; i < qtd; i++){
        printf("%i ", trajeto[i]); // substituir por escreve_lcd
    }
    printf("\n\nDist até o passageiro: %im",calcula_distancia(x_carro, y_carro, x_passageiro, y_passageiro)); // substituir por escreve_lcd
    printf("\nDist até o destino: %im\n\n",calcula_distancia(x_passageiro, y_passageiro, x_desti, y_desti)); // substituir por escreve_lcd
    printf("Em (%i, %i) seguir a: ", x,y); // substituir por escreve_lcd
    gps(x,y, x_passageiro, y_passageiro);
    
    return 0;
}
*/

ISR(USART_RX_vect){ // interrupção de recebimento serial
	while(!(UCSR0A& (1<<RXC0)));
	char temp =0;
	temp = UDR0;
	serial_global[contador_global] =  temp;
	contador_global++;
	if (contador_global==11){
		contador_global =0;
		limpa_lcd();
		escreve_lcd(serial_global);
	}
}

int main(void){
	char perfil;
	unsigned char verificacao = 0;						// flag de verificacao se é solicitado desligamento ou nao
	char flagSistema = 0;						// flag pra indicar se o sistema esta ligado ou nao
	char opcaoB = 2;							// por padrao a opcaoB será 2 = preco; 1 = menor dist até cliente; 3 = menor tempo de corrida
	char motoristaOcupado = 0;					// flag que apenas o operador 1 pode mudar, se o sistema indica ocupado ou nao em atendimento
	
	inicia();									//nao liga o display nem configura serial, aguarda comando do usuario
	while (1) {
		verificacao = verifica_login();
		if (verificacao == '*'){
			desligaSistema(&flagSistema);
		}
		if (verificacao == '#' && flagSistema == 0){
			ligaSistema(&flagSistema);
			perfil = login(&flagSistema, &opcaoB, &motoristaOcupado);
		}
	}
}
