/*=================================================================================================*/
//	UNIVERSIDADE FEDERAL DO RIO GRANDE DO SUL
//	ENG04475 - Microprocessadores I (2023/2)
//	Projeto 1 - UBERGS
//
// Alunos:
// Bruno Gevehr Fernandes (00335299)
// Caio Fernando Leite França (00330098)
// Thiago Arndt Schimit (00333710)
/*=================================================================================================*/

/*======================================  Bibliotecas  ============================================*/

#include <avr/io.h>
#include <avr/interrupt.h>

/*======================  Definicoes de constantes e variaveis globais  ===========================*/

// Senhas
#define SENHA1 "1253"					//Senha do operador 1
#define SENHA2 "8759"					//Senha do operador 2

// Display (PORTC)
#define RS 4						// Pino RS do display na PORTC4
#define EN 5						// Pino EN do display na PORTC5

// Teclado (PORTD e PORTB)
#define linha_max 7					// Ultimo pino em que as linhas estao conectadas (PORTD7)
#define botao1 2					// Coluna 1 do teclado
#define botao2 1					// Coluna 2 do teclado
#define botao3 0					// Coluna 3 do teclado

// Teclas do teclado
unsigned char teclado[4][3]={'1','2','3',
	'4','5','6',
	'7','8','9',
	'*','0','#'
};

// Mapa
#define QTD_RUASX 4											// Quantidade de ruas verticais
#define QTD_RUASY 3											// Quantidade de ruas horizontais
#define LARGURAX 40											// Maior espessura de rua vertical
#define LARGURAY 48											// Maior espessura de rua horizontal
#define MAIORQUADRAY 520									// Maior largura de quadra (referencia vertical)
#define MAIORQUADRAX 474									// Maior Largura de quadra (referencia horizontal)
unsigned short RUASX [QTD_RUASX] = {378, 814,  1288, 1754}; // Centro das ruas horizontais
unsigned short RUASY [QTD_RUASY] = {484, 1004, 1524};		// Centro das ruas verticais
unsigned short RUAS [12][2] = {								// Mapa de todas as esquinas no com o centro das ruas como referencia
	378, 484,
	814, 484,
	1288, 484,
	1754, 484,
	378, 1004,
	814, 1004,
	1288, 1004,
	1754, 1004,
	378, 1524,
	814, 1524,
	1288, 1524,
	1754, 1524
};

// Esquinas do mapa
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
	1790, 1580
};


#define MAX_CLIENTES 3					// Maximo de clientes em espera


/*======================================  Structs  ============================================*/

typedef struct posCarro{			// Struct para posicao do Carro
	short x;
	short y;
}posCarro;

typedef struct cliente{				// Struct para dados dos clientes
	unsigned short cod;				// Codigo do Cliente
	unsigned short pos_saida_x;		// Posicao x do Cliente
	unsigned short pos_saida_y;		// Posicao y do Cliente
	unsigned short pos_destino_x;	// Posicao x do Destino
	unsigned short pos_destino_y;	// Posicao y do Destino
	unsigned short distCliente;		// Distancia do Carro ate o Cliente
	unsigned short distDestino;		// Distancia do Cliente ate o Destino
	unsigned short precoEstimado;	// Preco estimado da corrida
	unsigned short tempoEstimado;	// tempo estimado da corrida
}cliente;

/*====================================  Variáveis Globais =======================================*/

char flagClienteGlobal;					// Flag global para indicar recebimento de cliente na serial
char flagClienteDisponivel;				// Flag global para indicar cliente disponivel apos aceitar corrida
unsigned short auxTempoCorridaGlobal;	// Contador auxiliar usado para calcular o tempo da corrida
unsigned short janela10secGlobal;		// Contador auxiliar usado para calcular o tempo da corrida, incrimentado a cada 10s
cliente bufferCliente;					// Buffer global para armazenamento de clientes lidos da serial
posCarro posCarroGlobal;				// Struct global para posicao do carro
unsigned char serial_global[20]={'\0'}; // Buffer para armazenar dados da serial
unsigned char contador_global=0;		// Contador auxiliar do buffer



/*======================================  Timers e Atrasos  =========================================*/
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

void atraso_2ms(){
	TCCR1B = 10;				// CTC prescaler de 8
	OCR1A = 4000;				// 4000 contagens (2ms)
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
	TCCR1B = 13;				// CTC prescaler de 1024    0000 1101
	OCR1A = 31250;				// 31250 contagens (2s)
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

void atraso_250ms(){
	TCCR1B = 13;				// CTC prescaler de 1024
	OCR1A = 3907;				// 3907 contagens (250ms)
	TCNT1 = 0;
	while((TIFR1 & (1<<1))==0);	// aguarda estouro
	TIFR1 = 1<<1;				// reseta flag de estouro
	
}

void startContadorTempo() {		//	Contador de tempo para contar o tempo de uma corrida
	TIFR0 = 1<<1;
	TCCR0B = 0x05;				// Configura o Timer0 no modo CTC e prescaler de 1024
	OCR0A = 124;
	TCNT0 = 0;
}

void stopContadorTempo() {
	TCCR0B = 0;
}


/*====================================  Funcoes da Serial =======================================*/

// Configuracao da serial
void config_serial(){
	UCSR0B |= (1 << RXCIE0);			// Habilita interrupção serial
	UBRR0H = 0;
	UBRR0L = 51;						// 19200bps
	UCSR0B = (1<<4);					// Liga serial
	UCSR0C = 0x06;						// 8bits +1 stopbit -> sem paridade
	UCSR0B |= (1 << TXEN0);				// Transmissao serial
	UCSR0B |= (1 << RXEN0);				// Recepcao serial
}

void desliga_serial(){
	UCSR0B &= 0x0;
}

void escreve_serial(char dado) {
	while (!(UCSR0A & (1 << UDRE0)));		// Aguarda o buffer de transmissão ficar vazio
	UDR0 = dado;							// Coloca o byte no buffer de transmissão
}

void string_serial (char  *msg){ 			// Escreve um string no serial
	unsigned char i=0;
	while (msg[i] != 0){
		escreve_serial(msg[i]);
		i++;
	}
}

void limpa_serial_global(){
	char i;
	for(i = 0; i<20; i++){
		serial_global[i] = '\0';
	}
	contador_global=0;
}

unsigned char le_serial(){				// funcao de leitura serial -> nao usada
	while ((UCSR0A & (1<<RXC0)) == 0);	// enquanto nao recebe leitura, aguarda
	UCSR0A |= (1<<RXC0);				// limpa flag
	return UDR0;						// retorna leitura
}

/*====================================  Configuracoes do LCD 16x2 =======================================*/

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
		atraso_40us();
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


/*====================================  Funcoes auxiliares =======================================*/

// Funcao para desligar o sistema quando solicitado
void desligaSistema (char *desligaSistema) {
	if (*desligaSistema == 1){
		desliga_lcd_4bits();
		desliga_serial();
		*desligaSistema = 0;
		string_serial("UE");
		escreve_serial(0);
	}
}

// Funcao para ligar o sistema quando solicitado
void ligaSistema(char *flagSistema) {
	if (*flagSistema == 0){					// se o sistema esta desligado
		inicia_lcd_4bits();
		escreve_lcd("Insira a senha:");
		comando_lcd(0xC0);					// nova linha
		config_serial();
		*flagSistema = 1;					// define como ligado
		atraso_1s();						// atraso de 1seg pra nao ser lido o que estiver sendo pressionado logo apos iniciar
		string_serial("UE");				// avisa que o motorista esta indisponivel (deslogado)
		escreve_serial(0);
	}
}

//Funcao que converte um dado valor para um caractere numérico em ASCII
void converteASCII (unsigned short valor, char *stringConvertida){
	char i = 0, b = 0;
	if (valor > 65534){
		stringConvertida[0] = '*';
		stringConvertida[1] = '\0';
		return;
	}
	if (valor == 0){
		stringConvertida[0] = '0';
		stringConvertida[1] = '\0';
		return;
	}
	
	unsigned short divisor = 10;
	unsigned short auxiliar = 0;
	// associa cada caractere a seu respectivo valor em ASCII
	for (i = 0; i < 5; i++){
		auxiliar = valor%(divisor);
		valor -= auxiliar;
		valor/=10;
		auxiliar += 48;
		stringConvertida[i]=auxiliar;
		auxiliar = 0;
	}
	while (stringConvertida[i-1] == '0'){
		stringConvertida[i-1] = '\0';
		i--;
	}
	i--;
	
	char inicio = 0;
	char fim = i;
	// Reordena a string
	while (inicio < fim){
		char letraAux = stringConvertida[inicio];
		stringConvertida[inicio] = stringConvertida[fim];
		stringConvertida[fim] = letraAux;
		inicio++;
		fim--;
	}
	if (stringConvertida[0]=='\0')
	stringConvertida[0] = '0';
	
	stringConvertida[5] = '\0';
	return;
}

// Funcao que imprime um valor na tela
void imprimeASCII (unsigned short valor){	
	char string[6];
	converteASCII(valor, string);
	escreve_lcd(string);
}

// Funcao que calcular a menor distancia (raiz dos quadrados) entre dois pontos
double distancia(double x1, double y1, double x2, double y2) {
	double diffX = x2 - x1;
	double diffY = y2 - y1;
	// Calcula o quadrado da distância
	double distanciaQuadrada = diffX * diffX + diffY * diffY;
	// Aproximação inicial para a raiz quadrada
	double aproximacao = distanciaQuadrada / 3.0;
	// Método de Newton para encontrar a raiz quadrada
	for (int i = 0; i < 50; ++i) {
		aproximacao = 0.5 * (aproximacao + distanciaQuadrada / aproximacao);
	}
	return aproximacao;
}

// Funcao que calcula o modulo de um numero
unsigned int modulo (int x){
	if (x < 0) {
		return -x;
		} else {
		return x;
	}
}

// Funcao que descobre as esquinas adjacentes a um ponto. Retorna um vetor com todas elas
char esquinas_adjacentes (unsigned short x, unsigned short y, unsigned char vet[]){
	unsigned char i, qtd = 0, flag = 0;
	unsigned short aux = x;
	
	while(x >= 0 && x <= 2500 && flag == 0){				// verifca quais esquinas em X maiores estao adjacentes
		x++;
		for (i = 0; i < 12; i++){
			if (x == RUAS[i][0] && y == RUAS[i][1]){
				vet[qtd] = i;
				qtd++;
				flag = 1;
			}
		}
	}
	flag = 0;
	x = aux;
	while(x >= 0 && x <= 2500 && flag == 0){				// verifca quais esquinas em X menores estao adjacentes
		x--;
		for (i = 0; i < 12; i++){
			if (x == RUAS[i][0] && y == RUAS[i][1]){
				vet[qtd] = i;
				qtd++;
				flag = 1;
			}
		}
	}
	flag = 0;
	x = aux;
	aux = y;
	while(y >= 0 && y <= 2500 && flag == 0){				// verifca quais esquinas em Y maiores estao adjacentes
		y++;
		for (i = 0; i < 12; i++){
			if (x == RUAS[i][0] && y == RUAS[i][1]){
				vet[qtd] = i;
				qtd++;
				flag = 1;
			}
		}
	}
	flag = 0;
	y = aux;
	while(y >= 0 && y <= 2500 && flag == 0){				// verifca quais esquinas em Y menores estao adjacentes
		y--;
		for (i = 0; i < 12; i++){
			if (x == RUAS[i][0] && y == RUAS[i][1]){
				vet[qtd] = i;
				qtd++;
				flag = 1;
			}
		}
	}
	return qtd;			// retorna a quantidade de esquinas encontradas
}

// Funcao que escolhe a esquina adjacente mais proxima do destino
unsigned char escolhe_esquina(unsigned short x, unsigned short y, unsigned short x_final, unsigned short y_final){
	unsigned char i, esquina = 0, qtd =0;;
	double menor_dist = 10000;
	unsigned char vet[4];
	qtd = esquinas_adjacentes(x,y,vet);
	for(i = 0; i< qtd; i++){
		if (menor_dist > distancia(RUAS[vet[i]][0], RUAS[vet[i]][1], x_final, y_final)){
			menor_dist = distancia(RUAS[vet[i]][0], RUAS[vet[i]][1], x_final, y_final);
			esquina = vet[i];
		}
	}
	return esquina;
}

// Funcao que calcula as esquinas de um trajeto
unsigned char calcula_caminho (unsigned short x, unsigned short y, unsigned short x_final, unsigned short y_final, unsigned char trajeto[]){
	unsigned char destino, flag =0, i = 0;
	destino = escolhe_esquina(x_final,y_final,x,y);				// escolhe a esquina do destino
	while (!flag){												// salva esquinas enquanto a esquina atual nao for a esquina do destino
		trajeto[i] = escolhe_esquina(x,y,x_final,y_final);
		x = RUAS[trajeto[i]][0];
		y = RUAS[trajeto[i]][1];
		i ++;		
		if (x == RUAS[destino][0] && y == RUAS[destino][1]){
			flag = 1;
		}
	}
	return i;
}

// Calcula a distancia de um trajeto (caminho efetivo em X e Y a ser realizadopelo motorista)
unsigned short calcula_distancia (unsigned short x_inicial, unsigned short y_inicial, unsigned short x_final, unsigned short y_final){
	unsigned short dist = 0;
	unsigned char esquina1, esquina2;
	if ( (modulo(y_final - y_inicial) < MAIORQUADRAY) && (x_final == x_inicial)){				// se eles estao na mesa rua e quadra em y
		dist = modulo( y_final - y_inicial);
		} else if ( (modulo(x_final - x_inicial) < MAIORQUADRAX) && (y_final == y_inicial)){	// se eles estao na mesa rua e quadra em x
		dist = modulo( x_final - x_inicial);
		} else {																				// senao,
		esquina1 = escolhe_esquina(x_inicial, y_inicial, x_final, y_final);						
		esquina2 = escolhe_esquina(x_final, y_final, x_inicial, y_inicial);						
		dist += modulo(x_inicial - RUAS[esquina1] [0]) + modulo (y_inicial - RUAS[esquina1][1]);				// ponto de origem ate 1ª esquina
		dist += modulo(x_final - RUAS[esquina2][0]) + modulo (y_final - RUAS[esquina2][1]);						// esquina do destino ate o destino
		dist += modulo(RUAS[esquina1][0] - RUAS[esquina2][0]) + modulo (RUAS[esquina1][1] - RUAS[esquina2][1]); // resto do trajeto
	}
	return dist;

}

// Funcao que indica o sentido que o carro deve seguir
void gps (unsigned short x, unsigned short y, unsigned short x_final, unsigned short y_final, char flagComCliente, char indiceInfo, unsigned short precoCorrida){
	unsigned char proxima = 0, sentido1 = 0, sentido2 = 0;
	unsigned short x_prox, y_prox, x_prox2, y_prox2;
	unsigned int dist_destino;
	proxima = escolhe_esquina(x, y, x_final, y_final);	// verifica proxima esquina
	
	// para indicar qual sentido seguir
	x_prox = RUAS[proxima][0];
	y_prox = RUAS[proxima][1];
	if (x == x_final){
		if (y < y_final){
			sentido1 = 'S';
			} else if (y > y_final){
			sentido1 = 'N';
			} else {
			sentido1 = 'D';
		}
		
	} else if (y == y_final){
		if(x < x_final){
			sentido1 = 'L';
		} else if (x > x_final){
			sentido1 = 'O';
		} else {
			sentido1 = 'D';
		}		
	} else if (x > x_prox){
		sentido1 = 'O';
	} else if (x < x_prox){
		sentido1 = 'L';
	} else if (y > y_prox){
		sentido1 = 'N';
	} else if (y < y_prox){
		sentido1 = 'S';
	}
	
	dist_destino = calcula_distancia(x,y,x_final,y_final);
	
	// para calcular em qual sentido virar a seguir
	if (x != x_final && y != y_final){
		x = x_prox;
		y = y_prox;
		proxima = escolhe_esquina(x, y, x_final, y_final);
		
		x_prox = RUAS[proxima][0];
		y_prox = RUAS[proxima][1];
		if((modulo(x_final-x_prox)>modulo(x_final-posCarroGlobal.x))||(modulo(y_final-y_prox)>modulo(y_final-posCarroGlobal.y))){
			x_prox=x;
			y_prox=y;
		}

		if (x == x_final){
			if (y < y_final){
				sentido2 = 'S';
				} else if (y > y_final){
				sentido2 = 'N';
				} else {
				sentido2 = 'D';
			}
		}else if (y == y_final){
			if(x < x_final){
				sentido2 = 'L';
			} else if (x > x_final){
				sentido2 = 'O';
			} else {
				sentido2 = 'D';
			}
		} else if (x > x_prox){
			sentido2 = 'O';
		} else if (x < x_prox){
			sentido2 = 'L';
		} else if (y > y_prox){
			sentido2 = 'N';
		} else if (y < y_prox){
			sentido2 = 'S';
		}		
	}
	if (indiceInfo == 0){					// mostra a direcao que se deve seguir na primeira linha
		limpa_lcd();
		escreve_lcd("Siga a ");
		if (sentido1 == 'N'){
			escreve_lcd("Norte.");
			} else if (sentido1 == 'S'){
			escreve_lcd("Sul.");
			} else if (sentido1 == 'L'){
			escreve_lcd("Leste.");
			} else if (sentido1 == 'O'){
			escreve_lcd("Oeste.");
		}
		} else if (indiceInfo == 1){		// mostra o preco da corrida na primeira linha
		limpa_lcd();
		escreve_lcd("Preco(cR$):");
		imprimeASCII(precoCorrida);
	}
	
	// na segunda linha, mostra a direcao se seguir
	if (posCarroGlobal.x != x_final && posCarroGlobal.y != y_final){
		comando_lcd(0xC0);
		escreve_lcd(" Em ");
		imprimeASCII(modulo(posCarroGlobal.y-y)+modulo(posCarroGlobal.x-x));
		escreve_lcd("m ");
		if (sentido2 == 'N'){
			escreve_lcd("Norte.");
			} else if (sentido2 == 'S'){
			escreve_lcd("Sul.");
			} else if (sentido2 == 'L'){
			escreve_lcd("Leste.");
			} else if (sentido2 == 'O'){
			escreve_lcd("Oeste.");
		}
	} else
	if (flagComCliente){
		comando_lcd(0xC0);
		escreve_lcd("Destino em: ");
		imprimeASCII(dist_destino);
		escreve_lcd("m.");
		} else if (!flagComCliente){
		comando_lcd(0xC0);
		escreve_lcd("Cliente em: ");
		imprimeASCII(dist_destino);
		escreve_lcd("m.");
	}
	
}

// Funcao que estima o preço em centavos
unsigned short estimagemPreco (unsigned short dist) {
	unsigned short preco = 200;
	preco = preco + (12*dist)/100;  // preço do km percorrido
	preco = preco + (dist/139)*5;   // preço do tempo (138,88 m/10s)
	
	return preco;
}

// Funcao que estima o tempo em segundos
unsigned short estimagemTempo (unsigned short dist) {
	unsigned short tempo = dist/14;
	return tempo; 		//50km/h = 13,88 m/s ; arredondei para 14 m/s para fins de aproximaçao
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

// Funcao que verifica se é pressionada a tecla '*' por tempo suficiente
unsigned char verificacao_tecla1(char tempo){ //tempo multiplicado por 2 em seg por conta de usar atraso de 0,5seg
	unsigned char cont=0;
	unsigned char ultimo=0;
	while(cont<tempo*4){
		if(ultimo==(1<<botao1)){
			atraso_1ms();
			cont++;
		}
		else{
			if(ultimo==(PINB&(1<<botao1))){
				atraso_250ms();
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

// Funcao que verifica se é pressionada a tecla '#' por tempo suficiente
unsigned char verificacao_tecla2(char tempo){  // tempo em segundos (aproximado
	unsigned char cont=0;
	unsigned char ultimo=0;
	while(cont<tempo*4){
		if(ultimo == (1<<botao3)){
			atraso_1ms();
			cont++;
		}
		else{
			if(ultimo == (PINB&(1<<botao3))){
				atraso_250ms();
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

//Funcao que verifia se é solicitado o logout, o desligue do sistema, ou a ligacao do sistema
unsigned char verifica_login(){
	
	PORTD&=~(1<<(3));
	if(!verificacao_tecla1(3)){
		PORTD|=(1<<(3));
		return teclado[3][0];
	}
	if(!verificacao_tecla2(2)){
		PORTD|=(1<<(3));
		return teclado[3][2];
	}
	PORTD|=(1<<(3));
	return '\0';
}

// Funcao qu escaneia as linhas 1, 2, 3 e 4
unsigned char scan(unsigned char linha){
	PORTD&=~(1<<(linha_max-linha));
	if(!debounce(botao1)){
		if(linha == 4){
			if(!verificacao_tecla1(3)){		//verifica se vem comando de desligar o sistema
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
		if(linha == 4){
			if(!verificacao_tecla2(2)){		//verifica se vem comando de logoff o sistema
				return 'x';
			}
		}
		while(!(PINB&(1<<botao3)));
		PORTD|=(1<<(linha_max-linha));
		return teclado[linha-1][2];
	}
	PORTD|=(1<<(linha_max-linha));
	return '\0';
}

// Funcao que configura as portas e interrupçoes iniciais
void inicia(){
	DDRD = 0x79;			//0111 1000 -> inicia porta D
	DDRB = 0x00;
	PIND=255;				// pullup porta D
	PINC=255;				// pullup porta C
	PINB=255;				// pullup porta B
	DDRC = 0x3F;			// porta C como saida -> 00111111
	TCCR1A = 0;					// modo padrão do timer
	sei();						// habilita interrupções
	UCSR0B |= (1 << RXCIE0);	// Habilita a interrupção de recepção serial
	TIMSK0 |= (1 << TOIE0);		// Habilita interrupcao no timer 0
	limpa_lcd();
	string_serial("UE");
	escreve_serial(0);
}

// Funcao que compara duas strings
char compara_string(char* a, char* b){ // 0 se igual, 1 se diferente
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

// Funcao que le a senha do usuario
char ler_senha(){
	char perfil = 0; //perfil 0 (senha invalida), perfil 1 (ADM), perfil 2 (OUTRO)
	char senha[10]={'\0'}, i=0, j=0, letra = '\0';
	char flagReset = 0;									// para conferir se o botao de reset foi apertado no Servidor Ubergs
	while(letra!='#'&&(j<10)){
		if (posCarroGlobal.x != 0 && flagReset == 0){	// caso o botao reset seja apertado (no Servidor), avisa que está insdispoivel
			string_serial("UE");
			escreve_serial(0);
			flagReset = 1;
		}		
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
				while (senha[j+1]!='\0'){
					senha[j] = '\0';
					j++;
				}
				j=0;
				limpa_lcd();
				escreve_lcd("Insira a senha:");
				comando_lcd(0xC0);
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

// Funcao para controlar o carro
void movimento_manual (char letra){
	if (letra == '5'){
		string_serial("UM");
		escreve_serial(0);
		} else if (letra == '2'){
		string_serial("UM");
		escreve_serial(1);
		} else if (letra == '6'){
		string_serial("UM");
		escreve_serial(2);
		} else if (letra == '4'){
		string_serial("UM");
		escreve_serial(3);
	} else
	return;
	
}

// Funcao que copia as informacoes de um cliente
void copiaCliente(cliente *clienteDestino, cliente *clienteOrigem){
	clienteDestino->cod = clienteOrigem->cod;
	clienteDestino->distCliente = clienteOrigem->distCliente;
	clienteDestino->distDestino = clienteOrigem->distDestino;
	clienteDestino->pos_destino_x = clienteOrigem->pos_destino_x;
	clienteDestino->pos_destino_y = clienteOrigem->pos_destino_y;
	clienteDestino->pos_saida_x = clienteOrigem->pos_saida_x;
	clienteDestino->pos_saida_y = clienteOrigem->pos_saida_y;
	clienteDestino->precoEstimado = clienteOrigem->precoEstimado;
	clienteDestino->tempoEstimado = clienteOrigem->tempoEstimado;
}

// Funcao que ordena uma lista de clientes
void ordenaClientes (cliente *clientesEspera, char opcaoB, char *quantidadeClientes){
	char i, j;
	cliente Aux;
	
	if (opcaoB == 1){														// ordena de acordo com a distCliente
		for (i = 0; i<*quantidadeClientes-1; i++){
			for(j = 0; j < *quantidadeClientes - i -1; j++){
				if (clientesEspera[j].distCliente > clientesEspera[j+1].distCliente){
					copiaCliente(&Aux, &clientesEspera[j]);
					copiaCliente(&clientesEspera[j], &clientesEspera[j+1]);
					copiaCliente(&clientesEspera[j+1], &Aux);
				}
			}
		}
		return;
	}

	if (opcaoB == 2){
		for (i = 0; i<*quantidadeClientes-1; i++){
			for(j = 0; j < *quantidadeClientes - i -1; j++){
				if (clientesEspera[j].precoEstimado < clientesEspera[j+1].precoEstimado){
					copiaCliente(&Aux, &clientesEspera[j]);
					copiaCliente(&clientesEspera[j], &clientesEspera[j+1]);
					copiaCliente(&clientesEspera[j+1], &Aux);
				}
			}
		}
		return;
	}

	if (opcaoB == 3){
		for (i = 0; i<*quantidadeClientes-1; i++){
			for(j = 0; j < *quantidadeClientes - i -1; j++){
				if (clientesEspera[j].tempoEstimado > clientesEspera[j+1].tempoEstimado){
					copiaCliente(&Aux, &clientesEspera[j]);
					copiaCliente(&clientesEspera[j], &clientesEspera[j+1]);
					copiaCliente(&clientesEspera[j+1], &Aux);
				}
			}
		}
		return;
	}
}

// Funcao que armazena o cliente do buffer em uma lista
void armazenaCliente(cliente *clientesEspera, char opcaoB, char *quantidadeClientes){
	if (!flagClienteGlobal)
	return;

	if (*quantidadeClientes == MAX_CLIENTES)	// se tiver todo o vetor preenchido não armazena mais clientes
	return;

	bufferCliente.distCliente = calcula_distancia(posCarroGlobal.x, posCarroGlobal.y, bufferCliente.pos_saida_x, bufferCliente.pos_saida_y);
	bufferCliente.distDestino = calcula_distancia(bufferCliente.pos_saida_x, bufferCliente.pos_saida_y, bufferCliente.pos_destino_x, bufferCliente.pos_destino_y);
	bufferCliente.precoEstimado = estimagemPreco(bufferCliente.distDestino);
	bufferCliente.tempoEstimado = estimagemTempo(bufferCliente.distDestino);
	
	copiaCliente(&clientesEspera[*quantidadeClientes], &bufferCliente);
	*quantidadeClientes += 1;
	
	limpa_lcd();
	escreve_lcd("Corrida:");
	imprimeASCII(bufferCliente.cod);
	comando_lcd(0xC0);
	escreve_lcd("recebida.");
	atraso_2s();
	
	ordenaClientes(clientesEspera, opcaoB, quantidadeClientes);
	
	flagClienteGlobal = 0;
	return;
}

// Funcao que zera as informaçoes de um cliente
void apagaCliente(cliente *clienteApagado){
	clienteApagado->cod = 0;
	clienteApagado->pos_destino_x = 0;
	clienteApagado->pos_destino_y = 0;
	clienteApagado->pos_saida_x = 0;
	clienteApagado->pos_saida_y = 0;
	clienteApagado->precoEstimado = 0;
	clienteApagado->tempoEstimado = 0;
	clienteApagado->distCliente = 0;
	clienteApagado->distDestino = 0;
}

// Funcao que remove um cliente da lista
void removeCliente(cliente *clienteEspera, char *quantidadeClientes, char *indiceCliente) {
	if (*quantidadeClientes == 0)
	return;
	
	char i = 0;
	for (i = *indiceCliente-1; i < *quantidadeClientes-1; i++){
		copiaCliente(&clienteEspera[i], &clienteEspera[i+1]);		// desloca todos os clientes do indice selecionado em diante a esquerda
	}
	*quantidadeClientes -=1;								// indica que diminuiu a quantidade de clientes
	apagaCliente(&clienteEspera[i]);						// coloca a ultima posicao como zero;
	
	if (*indiceCliente -1 == *quantidadeClientes)			//se removeu o ultimo cliente da lista, diminui o indice
	*indiceCliente-=1;
	
	return;
}

// Funcao que imprime a informacao desejada de um cliente no display
void printCliente (char indiceCliente, char indiceInfo, cliente *Cliente){
	
	limpa_lcd();
	escreve_lcd("Cliente:");
	imprimeASCII(Cliente[indiceCliente].cod);
	comando_lcd(0xC0); // nova linha
	
	if (indiceInfo == 0){	//preco estimado
		escreve_lcd("Preco(cR$):");
		imprimeASCII(Cliente[indiceCliente].precoEstimado);
		return;
	}
	if (indiceInfo == 1){	// tempo estimado
		escreve_lcd("Tempo:");
		imprimeASCII(Cliente[indiceCliente].tempoEstimado);
		letra_lcd('s');
		return;
	}
	if (indiceInfo == 2){	// dist ate cliente
		escreve_lcd("Dist Clnt:");
		imprimeASCII(Cliente[indiceCliente].distCliente);
		letra_lcd('m');
		return;
	}
	if (indiceInfo == 3){	// dist ate destino
		escreve_lcd("Trajeto:");
		imprimeASCII(Cliente[indiceCliente].distDestino);
		letra_lcd('m');
		return;
	}
}

// Funcao que imprime a direcao ou o preco atual da corrida no display
void printDirecao(char flagEmCorrida, char flagComCliente, cliente clienteAtual, char indiceInfo, unsigned short precoCorrida) {
	if (!flagEmCorrida){
		limpa_lcd();
		escreve_lcd("Posicao atual:");
		comando_lcd(0xC0);
		escreve_lcd("x: ");
		imprimeASCII(posCarroGlobal.x);
		escreve_lcd(" y: ");
		imprimeASCII(posCarroGlobal.y);
		return;
	}
	if (!flagComCliente) {		// se nao esta com cliente, guia ate o cliente
		gps(posCarroGlobal.x, posCarroGlobal.y, clienteAtual.pos_saida_x, clienteAtual.pos_saida_y, flagComCliente, indiceInfo, precoCorrida);
		return;
	}							// se ja esta com o cliente, guia ate o destino
	gps(posCarroGlobal.x, posCarroGlobal.y, clienteAtual.pos_destino_x, clienteAtual.pos_destino_y, flagComCliente, indiceInfo, precoCorrida);
	return;
}

// Menu interativo para o usuario (controlado por 8 e 0 para alterar os clientes, e 7 e 9 para mudar as informacoes de um cliente)
void menu(char *indiceCliente, char *indiceInfo, char quantidadeClientes, cliente *clientesEspera, char flagComCliente, char estadoMotorista, cliente clienteAtual, char flagEmCorrida, char letra, unsigned short precoCorrida){
	if (letra == '8'){
		if (!(*indiceCliente))			// nao faz nada
		return;
		if ((*indiceCliente)){			// ja estava mostrando o cliente no topo, agora mostra a direção a ser seguida
			*indiceCliente-=1;
			*indiceInfo = 0;
			printDirecao(flagEmCorrida, flagComCliente, clienteAtual, *indiceInfo, precoCorrida);
			return;
		}
		*indiceCliente-=1;
		*indiceInfo = 0;
		limpa_lcd();
		printCliente(*indiceCliente - 1, *indiceInfo, clientesEspera);
		imprimeASCII(*indiceCliente);
		return;
	}
	if (letra == '0' && flagEmCorrida == 0){
		if (*indiceCliente == quantidadeClientes)
		return;
		*indiceCliente+=1;
		*indiceInfo = 0;
		limpa_lcd();
		printCliente(*indiceCliente - 1, *indiceInfo, clientesEspera);
		imprimeASCII(*indiceCliente);
		return;
	}
	if (letra == '7'){
		if (!(*indiceInfo))
		return;
		*indiceInfo-=1;
		if (!(*indiceCliente)){
			printDirecao(flagEmCorrida, flagComCliente, clienteAtual, *indiceInfo, precoCorrida);
			return;
		}
		printCliente(*indiceCliente - 1, *indiceInfo, clientesEspera);
		return;
	}
	if (letra == '9'){
		if (*indiceInfo == 3)
		return;
		if (!(*indiceCliente)){
			if (*indiceInfo)
			return;
			if (flagComCliente){
				*indiceInfo = 1;
				printDirecao(flagEmCorrida, flagComCliente, clienteAtual, *indiceInfo, precoCorrida);
				return;
			}
			return;
		}
		*indiceInfo+=1;
		printCliente(*indiceCliente - 1, *indiceInfo, clientesEspera);
		return;
	}
	if (letra == '\0'){
		if (!(*indiceCliente)){
			printDirecao(flagEmCorrida, flagComCliente, clienteAtual, *indiceInfo, precoCorrida);
			return;
		}
		printCliente(*indiceCliente - 1, *indiceInfo, clientesEspera);
		return;
	}
}

// Funcao que verifica se o usuario aceito a corrida
void aceitaCorrida (char *indiceCliente, cliente *clienteAtual, cliente *clientesEspera, char *estadoMotorista, char *motoristaOcupado, char *flagEmCorrida, char *quantidadeClientes, char *letra, char opcaoB){

	if (*flagEmCorrida)	// se o motorista ja esta em atendimento
	return;
	if (*indiceCliente == 0)
	return;
	
	flagClienteDisponivel = 0;													// sempre limpa a flag indicando que o cliente esta disponivel
	
	if (*letra == '#'){															// se ele nao esta em atendimento, e aceita o cliente
		string_serial("UA");
		escreve_serial(clientesEspera[*indiceCliente-1].cod);					// avisa o servidor que aceitou
		atraso_500ms();
		while(!flagClienteDisponivel);											// aguarda resposta do servidor
		if (flagClienteDisponivel == 2){										//se servidor responde que cliente aceitou
			copiaCliente(clienteAtual, &clientesEspera[*indiceCliente-1]);		// copia as informacoes do cliente da lista de espera pro cliente atual
			removeCliente(clientesEspera, quantidadeClientes, indiceCliente);	// remove da lista de espera o cliente aceito
			*indiceCliente = 0;										// volta para a opcao que guia o carro
			*flagEmCorrida = 1;										// levanta a flag que esta em atendimento
			*letra = '\0';											// para evitar entrar em outra funcao, zera a letra
			limpa_lcd();											// avisa que aceitou o cliente
			escreve_lcd("   Cliente:");
			imprimeASCII(clienteAtual->cod);
			comando_lcd(0xC0);
			escreve_lcd("     aceito.");
			atraso_1s();											// aguarda um pouco para seguir
			if (*motoristaOcupado) {								// se a flag de que o motorista nao aparece ocupado em atendimento for verdadeira, retorna a funcao
				imprimeASCII(*motoristaOcupado);
				string_serial("UE");
				escreve_serial(1);
				*estadoMotorista = 1;
				return;
			}
			*estadoMotorista = 2;									// levanta a flag de ocupado do motorista
			string_serial("UE");
			escreve_serial(2);
			return;
			} else if (flagClienteDisponivel == 1){					// se o servidor responde como indisponivel, avisa o morotista
			limpa_lcd();
			escreve_lcd("Cliente Indisp");
			atraso_2s();
			return;
		}
	}
	if (*letra == '*'){											// recusa o cliente (aceitando, pegando e entregando o cliente instantaneamente)
		string_serial("UA");
		escreve_serial(clientesEspera[*indiceCliente-1].cod);
		string_serial("UI");
		escreve_serial(clientesEspera[*indiceCliente-1].cod);
		string_serial("UF");
		escreve_serial(clientesEspera[*indiceCliente-1].cod);
		removeCliente(clientesEspera, quantidadeClientes, indiceCliente);
		return;
	}
	return;
}

// Funcao que calcula o preco da corrida com os acumuladores globais
unsigned short calcula_precoCorrida(unsigned short dist){
	unsigned short preco = 200;								// tarifa base
	preco = preco + 120*(dist/1000);
	preco = preco + janela10secGlobal*5;
	return preco;
}

// Funcao que o usuario indica se o cliente embarcou ou desembarcou
void acaoPassageiro(char *estadoMotorista, cliente clienteAtual, char *flagComCliente, char *flagEmCorrida, unsigned short *precoCorrida,  char *letra, char *indiceInfo,
unsigned short *x_anterior, unsigned short *y_anterior, unsigned short *dist_corrida){
	if (*flagEmCorrida){						// se ele esta em corrida
		if (*letra == '#'){						// e clicou em aceitar
			if (!(*flagComCliente)){			// e sem o cliente
				*flagComCliente = 1;			// avisa que o cliente embarcou
				*x_anterior=posCarroGlobal.x;	// salva a posicao anterior para calculo do preco
				*y_anterior=posCarroGlobal.y;
				string_serial("UI");			// avisa o servidor
				escreve_serial(clienteAtual.cod);
				startContadorTempo();			// comeca a cronometrar o tempo de corrida
				atraso_500ms();	
				return;
			}
			if (*flagComCliente){			// cliente desembarcou
				*flagComCliente = 0;
				*flagEmCorrida = 0;
				*estadoMotorista = 1;
				*indiceInfo = 0;
				string_serial("UF");			// avisa o servidor
				escreve_serial(clienteAtual.cod);
				string_serial("UE");
				escreve_serial(*estadoMotorista);
				stopContadorTempo();
				*precoCorrida = calcula_precoCorrida(*dist_corrida);
				stopContadorTempo();
				janela10secGlobal = 0;			// zera o contador
				limpa_lcd();
				escreve_lcd("Fim da corrida");
				comando_lcd(0xC0);
				escreve_lcd("Preco(cR$):");		// informa o preco da corrida
				imprimeASCII(*precoCorrida);
				*letra  = '\0';
				atraso_2s();
				return;
			}
		}
	}
}

// Funcao que muda a ordem em que os clientes estao ordenados na lista
void mudaOpcaoB (char *opcaoB, cliente *clientesEspera, char *quantidadeClientes, char flagPerfil ,char letra){
	if (letra == '1' && flagPerfil == 1){
		*opcaoB += 1;
		if (*opcaoB > 3) *opcaoB = 1;
		ordenaClientes(clientesEspera, *opcaoB, quantidadeClientes);
		limpa_lcd();
		escreve_lcd("Ordenados por:");
		comando_lcd(0xC0);
		if (*opcaoB == 1){
			escreve_lcd("Menor Distancia");
			} else if (*opcaoB == 2){
			escreve_lcd("Maior Preco");
			} else if (*opcaoB == 3){
			escreve_lcd("Menor Tempo");
		}
		atraso_2s();
	}
}


// Funcao que muda se o motorista está ocupado ou nao enquanto estiver em atendimento
void mudaMotoristaOcupado (char *motoristaOcupado, char letra, char flagPerfil, char flagEmCorrida){
	if (letra == '3' && flagPerfil == 1){
		*motoristaOcupado = ~(*motoristaOcupado);
		limpa_lcd();
		if (!(*motoristaOcupado)){
			if (flagEmCorrida){
				string_serial("UE");
				escreve_serial(2);
			}
			escreve_lcd("Ocupado");
			} else if (*motoristaOcupado){
			if (flagEmCorrida){
				string_serial("UE");
				escreve_serial(1);
			}
			escreve_lcd("Disponivel");
		}
		comando_lcd(0xC0);
		escreve_lcd("em corridas.");
		atraso_2s();
	}
	
}

// Funcao que acumula a distancia da corrida, para calculo de preco
void atualiza_distancia_corrida(unsigned short *x_anterior, unsigned short *y_anterior, unsigned short *dist_corrida){
	*dist_corrida+= modulo(*x_anterior-posCarroGlobal.x)+modulo(*y_anterior-posCarroGlobal.y);
	*y_anterior=posCarroGlobal.y;
	*x_anterior=posCarroGlobal.x;
}


// Funcao que realiza as operacoes da UBERGS apos o login ser feito. Loop principal do programa.
char ubergs(char *flagSistema, char *opcaoB, char *motoristaOcupado, char *estadoMotorista, char flagPerfil, cliente *clientesEspera, char *quantidadeClientes){
	unsigned char verificacao = 0;
	char flagComCliente = 0;				// flag que indica se o motorista esta com cliente no carro
	char flagEmCorrida = 0;					// flag que indica se o motorista esta com corrida aceita
	flagClienteGlobal = 0;					// se ha cliente no buffer
	cliente clienteAtual;					// cliente sendo atendido no momento
	char indiceCliente = 0, indiceInfo = 0;	// indices do menu
	char i =0, letra;						// contador e letra apertada no teclado
	unsigned short precoCorrida = 0, dist_corrida = 0, x_anterior = 0, y_anterior = 0;	// variaveis da corrida
	
	*estadoMotorista = 1;
	string_serial("UE");					// avisa o servidor que esta disponivel
	escreve_serial(*estadoMotorista);	
	
	while (1){
		for (i = 1; i<=2; i++){				// escaneia as 2 primeiras linhas do teclado
			letra = scan (i);
			
			movimento_manual(letra);
			mudaOpcaoB(opcaoB, clientesEspera, quantidadeClientes, flagPerfil, letra);
			mudaMotoristaOcupado(motoristaOcupado, letra, flagPerfil, flagEmCorrida);
			
		}
		for (i=3; i<=4; i++){				// escaneia as 2 ultimas linhas do teclado
			letra = scan(i);
			
			menu(&indiceCliente, &indiceInfo, *quantidadeClientes, clientesEspera, flagComCliente, estadoMotorista, clienteAtual, flagEmCorrida, letra, precoCorrida);
			acaoPassageiro(estadoMotorista, clienteAtual, &flagComCliente, &flagEmCorrida, &precoCorrida, &letra, &indiceInfo, &x_anterior, &y_anterior, &dist_corrida);
			aceitaCorrida(&indiceCliente, &clienteAtual, clientesEspera, estadoMotorista, motoristaOcupado, &flagEmCorrida, quantidadeClientes, &letra, *opcaoB);
			
			if(indiceCliente == 0 && flagEmCorrida == 0){	// verifica solictacoes de logoff e desligamento do sistema
				if (letra == 'x'){
					string_serial("UE");
					escreve_serial(0);
					limpa_lcd();
					escreve_lcd("Logoff realizado");
					*estadoMotorista = 0;
					atraso_2s();
					return 1;
				}
				if (letra == 'd'){
					string_serial("UE");
					escreve_serial(0);
					*estadoMotorista = 0;
					desligaSistema(flagSistema);
					return letra;
				}
			}
		}
		if (*estadoMotorista == 1) {
			armazenaCliente(clientesEspera, *opcaoB, quantidadeClientes);	//armazena na lista de espera o cliente que esta no buffer, se houver
			flagClienteGlobal = 0;
		}
		
		if (flagComCliente){												// se esta com o cliente
			atualiza_distancia_corrida(&x_anterior,&y_anterior,&dist_corrida);
			precoCorrida = calcula_precoCorrida(dist_corrida);

		}
	}
}

// Funcao que ajusta o display para insercao da senha
void lcdEscreverSenha(){
	limpa_lcd();
	escreve_lcd("Insira a senha:");
	comando_lcd(0xC0); // nova linha
}

// Funcao em que se realiza login
char login (char *flagSistema, char *opcaoB, char *motoristaOcupado, char *estadoMotorista){
	char perfil = 0;
	lcdEscreverSenha();
	cliente clientesEspera[MAX_CLIENTES];
	char aux = 0;								
	for (aux = 0; aux < MAX_CLIENTES; aux++){		// limpa a lista de clientes em espera
		apagaCliente(&clientesEspera[aux]);
	}
	unsigned char quantidadeClientes = 0;
	
	while(1){
		perfil = ler_senha();
		if (perfil == 'd'){				//comando de desligar o sistema
			desligaSistema(flagSistema);
			return perfil;
		}
		if (perfil == 1){
			limpa_lcd();
			escreve_lcd("Perfil 1 Logado!");
			atraso_2s();
			perfil = ubergs(flagSistema, opcaoB, motoristaOcupado, estadoMotorista, 1, clientesEspera, &quantidadeClientes);
			quantidadeClientes = 0;
			lcdEscreverSenha();
		}
		if (perfil == 2){
			limpa_lcd();
			escreve_lcd("Perfil 2 Logado!");
			atraso_2s();
			perfil = ubergs(flagSistema, opcaoB, motoristaOcupado, estadoMotorista, 2, clientesEspera, &quantidadeClientes);
			quantidadeClientes = 0;
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

// Funcao que interpreta as mensagens enviadas pelo servidor externo, chamada pela interrupcao
void interpreta_serial(){ 
	unsigned char i;
	if (contador_global > 12 || serial_global[0] != 'S'){		// limpa qualquer lixo de memoria ou erro lido na serial
		limpa_serial_global();
	}
	
	else if (serial_global[0] == 'S' && serial_global[1] == 'P' && serial_global[5] !='\0'){	 //Protocolo de posiçao do veículo
		string_serial("UP");		
		posCarroGlobal.x = (serial_global[2]<<8) + serial_global[3];		// converte para valores
		posCarroGlobal.y = (serial_global[4]<<8) + serial_global[5];

		// correcao de margens para considerar sempre no centro da rua
		for (i = 0; i < QTD_RUASX; i++){									
			if ((posCarroGlobal.x  > RUASX[i] - LARGURAX/2) && (posCarroGlobal.x  < RUASX[i] + LARGURAX/2)) posCarroGlobal.x  = RUASX[i];
		}
		for (i = 0; i < QTD_RUASY; i++){
			if ((posCarroGlobal.y > RUASY[i] - LARGURAY/2) && (posCarroGlobal.y < RUASY[i] + LARGURAY/2)) posCarroGlobal.y = RUASY[i];
		}
		limpa_serial_global();
	}
	
	else if(serial_global[0] == 'S' && serial_global[1] == 'C' && serial_global[10] !='\0'){ //protocolo de chamada de novo cliente
		bufferCliente.cod = serial_global[2];							// converte para valores
		bufferCliente.pos_saida_x = (serial_global[3]<<8) + serial_global[4];
		bufferCliente.pos_saida_y = (serial_global[5]<<8) + serial_global[6];
		bufferCliente.pos_destino_x = (serial_global[7]<<8) + serial_global[8];
		bufferCliente.pos_destino_y = (serial_global[9]<<8) + serial_global[10];
		flagClienteGlobal = 1;											// indica que cliente foi recebido no buffer
		
		// correcao de margens para considerar sempre no centro da rua
		for (i = 0; i < QTD_RUASX; i++){
			if ((bufferCliente.pos_saida_x  > RUASX[i] - LARGURAX/2) && (bufferCliente.pos_saida_x  < RUASX[i] + LARGURAX/2)) bufferCliente.pos_saida_x  = RUASX[i];
		}
		for (i = 0; i < QTD_RUASY; i++){
			if ((bufferCliente.pos_saida_y > RUASY[i] - LARGURAY/2) && (bufferCliente.pos_saida_y < RUASY[i] + LARGURAY/2)) bufferCliente.pos_saida_y = RUASY[i];
		}
		for (i = 0; i < QTD_RUASX; i++){
			if ((bufferCliente.pos_destino_x  > RUASX[i] - LARGURAX/2) && (bufferCliente.pos_destino_x  < RUASX[i] + LARGURAX/2)) bufferCliente.pos_destino_x  = RUASX[i];
		}
		for (i = 0; i < QTD_RUASY; i++){
			if ((bufferCliente.pos_destino_y > RUASY[i] - LARGURAY/2) && (bufferCliente.pos_destino_y < RUASY[i] + LARGURAY/2)) bufferCliente.pos_destino_y = RUASY[i];
		}
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
		flagClienteDisponivel = 2;	//cliente disponivel
		limpa_serial_global();
	}
	else if(serial_global[0] == 'S' && serial_global[1] == 'A' && serial_global[2] =='X'){ //protocolo de resposta do servidor a comando de aceite - não disponivel
		flagClienteDisponivel = 1;		//cliente nao disponivel
		limpa_serial_global();
	}
	else if(serial_global[0] == 'S' && serial_global[1] == 'I'){		// protocolo de resposta do servidor a comando de inicio de corrida - pegou cliente
		limpa_serial_global();
	}
	else if(serial_global[0] == 'S' && serial_global[1] == 'F'){		// protocolo de resposta do servidor a comando de fim de corrida - entregou cliente
		limpa_serial_global();
	}
}

// interrupção de recebimento serial
ISR(USART_RX_vect){
	while(!(UCSR0A& (1<<RXC0)));
	char temp =0;
	temp = UDR0;
	serial_global[contador_global] =  temp;
	contador_global++;
	interpreta_serial();
}

// interrupção do timer 0
ISR(TIMER0_OVF_vect){
	startContadorTempo();
	auxTempoCorridaGlobal++;
	if (auxTempoCorridaGlobal >= 625){	// a cada 10s, incrementa janela10secGlobal.
		auxTempoCorridaGlobal = 0;
		janela10secGlobal++;
	}
}


int main(void){
	char perfil;								// indica qual perfil esta logado
	char verificacao      = 0;					// flag de verificacao se é solicitado desligamento ou nao
	char flagSistema      = 0;					// flag pra indicar se o sistema esta ligado ou nao
	char opcaoB = 2;							// por padrao a opcaoB será 2 = preco; 1 = menor dist até cliente; 3 = menor tempo de corrida
	char motoristaOcupado = 0;					// flag que apenas o operador 1 pode mudar, se o sistema indica ocupado ou nao em atendimento, 0 = nao ocupado, 1 = ocupado
	char estadoMotorista  = 0;					// flag indicando estado do motorisra, 0 = indisponivel, 1 = disponivel, 2 = ocupado
	flagClienteGlobal	  = 0;					
	flagClienteDisponivel = 0;
	auxTempoCorridaGlobal = 0;
	janela10secGlobal = 0;
	
	inicia();									//nao liga o display nem configura serial, aguarda comando do usuario
	while (1) {
		verificacao = verifica_login();
		if (verificacao == '*'){
			desligaSistema(&flagSistema);
		}
		if (verificacao == '#' && flagSistema == 0){
			ligaSistema(&flagSistema);
			perfil = login(&flagSistema, &opcaoB, &motoristaOcupado, &estadoMotorista);
		}
	}
}
