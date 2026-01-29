/*
22/01/2026
código responsavel por:
fazer a leitura dos valores brutos do adc provenientes da esp, via comunicação serial;
calibração desses valores via regressão linear para se encontrar as correntes;
publicar em topico do ros2 esses dados das correntes eletricas;
*/

#include <stdio.h> //printf
#include <iostream>
#include <vector>

// strings
#include <cstring>
#include <string>
#include <sstream> // Permite tratar strings como se fossem fluxos de dados (streams)

// bibliotecas comunicação serial
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

const char* SERIAL_PORT = "/dev/ttyTHS1";

void serial_config (int serial_fd);
void leitura_serial (int serial_fd);
void publish_data (const std::vector<short int>& data); //funcao para debug
void processar_calibracao (const std::vector<short int>& data);

int main(void){

    int chave_serial = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if (chave_serial < 0) {
        perror("Erro fatal ao abrir serial"); 
        return 1; 
    }
    serial_config (chave_serial);

    while(true){
        leitura_serial (chave_serial); //loop para ficar lendo a uart
    }

    return 0;
}

void serial_config (int serial_fd){

    if (serial_fd < 0) { //teste para saber se abriu a porta
        printf("Error %i from fcntl:open: %s\n", errno, strerror(errno));
        return;
    }

    struct termios tty;

    if ( tcgetattr(serial_fd, &tty) != 0) { // atribui a config atual e testa se deu certo
        printf("Error %i from tcGETattr: %s\n", errno, strerror(errno));
    }

    /* configuracao da struct */

    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication
    tty.c_cflag &= ~CSIZE;  // Clear all the size bits
    tty.c_cflag |= CS8;     // 8 bits transmitted per byte
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag |= ICANON; // ativa modo canonico
    tty.c_iflag |= IGNCR; // ignore carriage return 
    cfsetispeed(&tty, B115200); // set baud rate to 115200 Hz
    cfsetospeed(&tty, B115200);

    /* configuracao da struct */

    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0 ){ // Save tty settings, also checking for error
        printf("Error %i from tcSETattr: %s \n", errno, strerror(errno));
    }

}

void leitura_serial (int serial_fd){

    char read_buffer[256];
    int n_bytes = read(serial_fd, &read_buffer, sizeof(read_buffer));   // retorna o numero de bytes lidos

    if (n_bytes > 0) {
        std::string dados_brutos(read_buffer, n_bytes);             // transformando vetor de char em string
        std::stringstream stream_completa(dados_brutos);            // string para stream
        std::string pacote, adc_string;                             // string para separar cada pacote de dados, string dos dados


        while(std::getline(stream_completa, pacote, '\n')){         // loop externo, separa todo dado recebido em pacotes
            std::stringstream stream_pacote(pacote);
            std::vector<short int> valores_lidos;                       // vetor com todos os dados lidos ja separados
               
            while (std::getline(stream_pacote, adc_string, ',')){ // loop interno, separa cada dado do pacote
                try {
                    valores_lidos.push_back(std::stoi(adc_string));
                }
                catch (...){
                    printf("Erro em transformar a string dos dados do adc em inteiro ");
                }
            }

            if(valores_lidos.size()==4){        //chama a funcao que envia os dados para fora quando o pacote esta completo
                publish_data(valores_lidos);
            }
        }
    }

}

void publish_data(const std::vector<short int>& data){
    std::cout << "Publicando -> Carga1: " << data[0] 
              << " | Carga2: " << data[1] 
              << " | Carga3: " << data[2] 
              << " | Carga4: " << data[3]<< std::endl;
}

void processar_calibracao (const std::vector<short int>& data){

    // Coeficientes (Idealmente seriam membros de uma classe, mas ok aqui)
    double a1=0.001580, a2=0.001580, a3=0.001580, a4=0.001580; 
    double b1=0.248780, b2=0.248780, b3=0.248780, b4=0.248780; 

    std::vector<float> correntes_reais;

    // Aplica a formula y = ax + b e guarda no novo vetor
    correntes_reais.push_back(a1 * data[0] + b1);
    correntes_reais.push_back(a2 * data[1] + b2);
    correntes_reais.push_back(a3 * data[2] + b3);
    correntes_reais.push_back(a4 * data[3] + b4);

}
