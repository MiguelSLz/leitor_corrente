/*
22/01/2026 - Miguel Sória da Luz
código responsavel por:
fazer a leitura dos valores brutos do adc provenientes da esp, via comunicação serial;
calibração desses valores via regressão linear para se encontrar as correntes;
publicar em topico do ros2 esses dados das correntes eletricas;
*/

// bibliotecas comunicação serial
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

const char* SERIAL_PORT = "/dev/ttyTHS1";

void serial_config ();

int main(void){

    

    return 0;
}

void serial_config (){
    int chave_serial = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
    
}