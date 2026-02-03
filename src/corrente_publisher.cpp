/*
22/01/2026
código responsavel por:
fazer a leitura dos valores brutos do adc provenientes da esp, via comunicação serial;
aplicar a formula utilizando os coeficientes encontrando na calibração para se encontrar as correntes;
publicar em topico do ros2 esses dados das correntes eletricas;
*/

#include <iostream> // cout, debug

// dados
#include <cstring>
#include <string>
#include <sstream> // Permite tratar strings como se fossem fluxos de dados (streams)
#include <vector>

// bibliotecas comunicação serial
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

//ros2
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float32_multi_array.hpp"
#include <chrono> 


class LeitorCorrenteNode : public rclcpp::Node{
    public:
        LeitorCorrenteNode();

    private:
        const char* SERIAL_PORT = "/dev/ttyTHS1";
        int chave_serial_;

        rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr publisher_;
        rclcpp::TimerBase::SharedPtr timer_;

        double a1=0.001580, a2=0.001580, a3=0.001580, a4=0.001580; // coeficientes angulares
        double b1=0.248780, b2=0.248780, b3=0.248780, b4=0.248780; // coeficientes lineares

        void serial_config ();
        void leitura_serial ();
        void publish_data (const std::vector<float>& data); //funcao para debug
        void processar_correntes (const std::vector<short int>& data);

    };

LeitorCorrenteNode::LeitorCorrenteNode() : Node("leitor_corrente") {
    chave_serial_ = open(SERIAL_PORT, O_RDWR | O_NOCTTY); //abre a porta

    if (chave_serial_ < 0) {
        RCLCPP_FATAL(this->get_logger(), "Erro fatal ao abrir serial: %s", strerror(errno));
    }else{
        this->serial_config(); // chama a config inicial
    }
        
    publisher_= this->create_publisher<std_msgs::msg::Float32MultiArray>("correntes_eletricas", 10); //publisher padrao ros2
    timer_ = this->create_wall_timer( //timer padrao ros2, chama leitura_serial na frequencia de 20 Hz
        std::chrono::milliseconds(50),
        std::bind(&LeitorCorrenteNode::leitura_serial, this));
}


void LeitorCorrenteNode::serial_config (){

    struct termios tty;

    if ( tcgetattr(chave_serial_, &tty) != 0) { // atribui a config atual e testa se deu certo
        RCLCPP_FATAL(this->get_logger(), "Error %i from tcGETattr: %s", errno, strerror(errno));
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

    if (tcsetattr(chave_serial_, TCSANOW, &tty) != 0 ){ // Save tty settings, also checking for error
        RCLCPP_FATAL(this->get_logger(), "Error %i from tcSETattr: %s", errno, strerror(errno));
    }

}

void LeitorCorrenteNode::leitura_serial (){

    char read_buffer[256];
    int n_bytes = read(chave_serial_, &read_buffer, sizeof(read_buffer));   // retorna o numero de bytes lidos

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
                    RCLCPP_WARN(this->get_logger(), "Erro parsing int");
                }
            }

            if(valores_lidos.size()==4){        //chama a funcao que envia os dados para fora quando o pacote esta completo
                this->processar_correntes(valores_lidos);
            }
        }
    }

}

void LeitorCorrenteNode::publish_data(const std::vector<float>& data){

    auto mensagem = std_msgs::msg::Float32MultiArray(); //instancia
    mensagem.data=data; // preenche os dados
    publisher_->publish(mensagem); // publica

    //debug
    RCLCPP_INFO(this->get_logger(), 
        "Publicando -> Carga1: %.3f A | Carga2: %.3f A | Carga3: %.3f A | Carga4: %.3f A", 
        data[0], data[1], data[2], data[3]);

}

void LeitorCorrenteNode::processar_correntes (const std::vector<short int>& data){

    std::vector<float> correntes;

    correntes.push_back(a1 * data[0] + b1);
    correntes.push_back(a2 * data[1] + b2);
    correntes.push_back(a3 * data[2] + b3);
    correntes.push_back(a4 * data[3] + b4);

    this->publish_data(correntes);

}


int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<LeitorCorrenteNode>());
    rclcpp::shutdown();
    return 0;
}
