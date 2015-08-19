#include <Wire.h>
#include "DHT.h"
#include "RTClib.h"
#include <String.h>
#include <Adafruit_BMP085.h>

#define ANEMOMETRO 0
#define BAROMETRO 1
#define HIGROMETRO 2
#define PIRANOMETRO 3
#define PLUVIOMETRO 4
#define TERMOMETRO 5
#define DEBUG true
#define id 10

#define piranometro A0
#define anemometro A1
#define pluviometro (22)
#define DHTPIN A1 
#define DHTTYPE DHT11 // modelo do higrometro
RTC_DS1307 rtc;

volatile int contaPulso = 0;
volatile int contaPulso2 = 0;
double  quantidadeChuva = 0.414;
volatile unsigned long ultContada = 0;
volatile unsigned long agora;
volatile long tmpUltContada;
unsigned long inicioTempo = 0;


DHT dht(DHTPIN, DHTTYPE, 30);
Adafruit_BMP085 bmp;

typedef struct {
  int sensor;
  double valor;
  String horario;
  String data;
} Sensor;


int i = 0;
String horario;
String data;
double valorAnemo;
double valorBarometro;
double valorHigrometro;
double valorPiranometro;
double valorPluviometro;
double valorTermometro;
int aux=0;
Sensor leitura[10];

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  delay(150);
  sendData("AT+RST\r\n","ready", 2000); 
  while(Serial.find("ready")){
    sendData("AT+RST\r\n","ready", 2000);
  }
  sendData("AT+CWMODE=1\r\n","no change", 2000); 
  while(Serial.find("no change")){
  sendData("AT+CWMODE=1\r\n","no change", 2000); 
  }  
  sendData("AT+CIPMODE=0\r\n","OK",1000);  
  while(Serial.find("OK")){
  sendData("AT+CIPMODE=0\r\n","OK",1000);  
  }  
  sendData("AT+CIPMUX=0\r\n","OK",1000); 
  while(Serial.find("OK")){
  sendData("AT+CIPMUX=0\r\n","OK",1000); 
  }  
  sendData("AT+CWJAP=\"ferreira\",\"313919familia\"\r\n","OK",10000); // SSID e Password da sua rede
  while(Serial.find("OK")){
  sendData("AT+CWJAP=\"ferreira\",\"313919familia\"\r\n","OK",10000); 
  }
  
  
  pinMode(pluviometro, INPUT);// 
  attachInterrupt(pluviometro, incpulso, RISING); //Configura o pino de sinal para trabalhar como interrupção
  
  bmp.begin();
  dht.begin();
}

void loop() {

 Serial.print(i);
  for(i=0;i<1;i++){
    lerAnemometro();
    lerBarometro();
    lerHigrometro();
    lerPiranometro();
    lerPluviometro();
    lerTermometro();
}   
   geraXML();
   
   i=0; 

}


void enviarLeitura(char* xml){

  sendData("AT+CIPSTART=\"TCP\",\"192.168.1.28\",80\r\n","Linked",10000); // colocar o IP do seu servidor
  char registro[3500] = "POST \/site\/recebe.php HTTP\/1.1\r\nHost: 192.168.0.103\r\nContent-Type: application\/x-www-form-urlencoded\r\nContent-Length: ";
  char complemento[2500]="";
  String tamanhoXML = (String) strlen(xml);
  tamanhoXML.toCharArray(complemento, sizeof(complemento));
  concatenar(registro,complemento, sizeof(complemento));
  strcpy(complemento,"\r\n\r\n");
  concatenar(registro,complemento, sizeof(complemento));
  strcpy(complemento,"xml=");
  concatenar(registro,complemento, sizeof(complemento));
  strcpy(complemento,xml);
  concatenar(registro,complemento, sizeof(complemento));
  strcpy(complemento,"\r\n\r\n");
  concatenar(registro,complemento, sizeof(complemento));
  char sendCIP[3000] = "AT+CIPSEND=";
  String tamanhoRegistro = (String) strlen(registro);
  tamanhoRegistro.toCharArray(complemento,sizeof(complemento));
  concatenar(sendCIP,complemento, sizeof(complemento));
  strcpy(complemento,"\r\n");
  concatenar(sendCIP,complemento, sizeof(complemento));
  sendData(sendCIP,">",1000);   //  vou mandar xx bytes...
  sendData(registro,"OKOKOK",100000);
//  Serial.print("\nstring enviada: ");
//  Serial.println(registro);
  
}

void gravar(int codSensor, double valor, String horario, String data) {
  leitura[aux].sensor = codSensor;
  leitura[aux].valor = valor;
  leitura[aux].horario = horario;
  leitura[aux].data = data;
  aux++;
  }
  
void lerAnemometro() {

  valorAnemo = analogRead(anemometro) - 121; 
  if (valorAnemo < 1023) {
    valorAnemo = (valorAnemo * 32.4) / 1023;
  }
  else {
    valorAnemo = 32.4;
  }
    horario = lerHora(); 
    data = lerData();
  gravar(ANEMOMETRO, valorAnemo,horario,data);

}

void lerBarometro() {

  valorBarometro = bmp.readPressure()/100;
    horario = lerHora(); 
    data = lerData();
  
  gravar(BAROMETRO,valorBarometro,horario,data);

}

void lerHigrometro() {

  valorHigrometro = dht.readHumidity();
    horario = lerHora(); 
    data = lerData();

  gravar(HIGROMETRO, valorHigrometro,horario,data);

}
void lerPiranometro() {
  delay(500);
   valorPiranometro = analogRead(piranometro) * 3.924; // constante calibrada com sensor de luminosidade profissional
   horario = lerHora(); 
   data = lerData();
   
  gravar(PIRANOMETRO, valorPiranometro,horario,data);

}

void lerPluviometro() {

  if (contaPulso == 1)inicioTempo = micros();
    if (contaPulso != contaPulso2) {
      contaPulso2 = contaPulso;
      valorPluviometro = ((contaPulso * quantidadeChuva) * 0.06); // 0.06 eh a constante de conversão para um recepiente de 1m X 1m
        }
      horario = lerHora(); 
      data = lerData();
      gravar(PLUVIOMETRO, valorPluviometro,horario,data);
}

void lerTermometro(){
    valorTermometro=dht.readTemperature();
    horario = lerHora(); 
    data = lerData();
    gravar(TERMOMETRO,valorTermometro,horario,data);    
}

String lerHora(){
  DateTime now = rtc.now();
  leitura[aux].horario=(String) now.hour();
  leitura[aux].horario+=":";
  leitura[aux].horario+=(String) now.minute();
  leitura[aux].horario+=":";    
  leitura[aux].horario+=(String) now.second();
  delay(1000); 
  return leitura[aux].horario;
    
}  
String lerData(){
 DateTime now = rtc.now();
 leitura[aux].data=(String) now.year();
 leitura[aux].data+="-";  
 leitura[aux].data+=(String) now.month();
 leitura[aux].data+="-";
 leitura[aux].data+=(String) now.day();
 return leitura[aux].data;
  
}

String sendData(String command, String fimResposta, const int timeout){
  
    if(DEBUG){
      Serial.print("\r\nEnviado: ");
      Serial.println(command);
      Serial.print("Resposta: ");
    }
    String response = "";
    Serial1.print(command); 
    long int time = millis();
    while( (time+timeout) > millis() && (response.indexOf(fimResposta)==-1)){
   
          while(Serial1.available()) {
     
        char c = Serial1.read(); 
        response+=c;
        if(DEBUG)Serial.print(c);
      } 
    }
    return response;
}

void geraXML (){
 
  char xml[3500]="";
  char complemento[2500]="";
 
    concatenar(xml,complemento, sizeof(complemento));
    strcpy(xml,"\<estacao>\n");
    for(i=0;i<6;i++){
    strcpy(complemento,"<registro>\n");
    concatenar(xml,complemento, sizeof(complemento));
    strcpy(complemento,"<id>");
    concatenar(xml,complemento, sizeof(complemento));
    String auxID = (String) id;
    auxID.toCharArray(complemento,sizeof(complemento));
    concatenar(xml,complemento, sizeof(complemento));
    strcpy(complemento,"<\/id>\n");
    concatenar(xml,complemento, sizeof(complemento));
    strcpy(complemento,"<sensor>");
    concatenar(xml,complemento, sizeof(complemento));
    String auxSensor = (String) leitura[i].sensor;
    auxSensor.toCharArray(complemento,sizeof(complemento));
    concatenar(xml,complemento, sizeof(complemento));
    strcpy(complemento,"<\/sensor>\n");
    concatenar(xml,complemento, sizeof(complemento));
    
    strcpy(complemento,"<data>");
    concatenar(xml,complemento, sizeof(complemento));
    leitura[i].data.toCharArray(complemento,sizeof(complemento));
    concatenar(xml,complemento, sizeof(complemento));
    strcpy(complemento,"<\/data>\n");
    concatenar(xml,complemento, sizeof(complemento));
    
    strcpy(complemento,"<hora>");
    concatenar(xml,complemento, sizeof(complemento));
    leitura[i].horario.toCharArray(complemento,sizeof(complemento));
    concatenar(xml,complemento, sizeof(complemento));
    strcpy(complemento,"<\/hora>\n");
    concatenar(xml,complemento, sizeof(complemento));
    strcpy(complemento,"<valorLido>");
    concatenar(xml,complemento, sizeof(complemento));
    String auxValor = (String) leitura[i].valor;
    auxValor.toCharArray(complemento,sizeof(complemento));
    concatenar(xml,complemento, sizeof(complemento));
    strcpy(complemento,"<\/valorLido>\n");
    concatenar(xml,complemento, sizeof(complemento));
    
    strcpy(complemento,"<\/registro>\n");
    concatenar(xml,complemento, sizeof(complemento));
       
  }
   strcpy(complemento,"<\/estacao>\n");
   concatenar(xml,complemento, sizeof(complemento));
   strcpy(complemento,"fim\n");
   concatenar(xml,complemento, sizeof(complemento));
   
   enviarLeitura(xml);

}

void concatenar(char* base, char* acrescimo, int tamAcres){
  int i;
  int tamBase = 0;
  while(base[tamBase]!='\0')tamBase++;
  for(i=0; i < tamAcres; i++){
     base[tamBase+i] =  acrescimo[i];  
  }
}

void incpulso ()
{

  agora = micros();
  tmpUltContada = abs(agora - ultContada);

  if (tmpUltContada > 200000L) {
    contaPulso++; //Incrementa a variável de contagem dos pulsos
    ultContada = agora;
  }
}




